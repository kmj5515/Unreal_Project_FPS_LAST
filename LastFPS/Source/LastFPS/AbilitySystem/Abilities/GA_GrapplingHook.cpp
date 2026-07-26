#include "AbilitySystem/Abilities/GA_GrapplingHook.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToActorForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Character/Components/LastFPSGrapplingAnimationComponent.h"
#include "Character/Components/LastFPSGrapplingTargetingComponent.h"
#include "Character/LastFPSHero.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Curves/CurveFloat.h"
#include "Data/Abilities/LastFPSGrapplingHookData.h"
#include "Engine/World.h"
#include "GameFramework/RootMotionSource.h"
#include "Utility/LastFPSTags.h"

namespace
{
	ERootMotionFinishVelocityMode ResolveFinishVelocityMode(
		const ELastFPSGrappleFinishVelocityMode FinishVelocityMode)
	{
		switch (FinishVelocityMode)
		{
		case ELastFPSGrappleFinishVelocityMode::Stop:
			return ERootMotionFinishVelocityMode::SetVelocity;
		case ELastFPSGrappleFinishVelocityMode::Clamp:
			return ERootMotionFinishVelocityMode::ClampVelocity;
		case ELastFPSGrappleFinishVelocityMode::Maintain:
		default:
			return ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
		}
	}
}

UGA_GrapplingHook::UGA_GrapplingHook()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_GrapplingHook);
	Tags.AddTag(LastFPSGameplayTags::Input_GrapplingHook);
	SetAssetTags(Tags);
}

void UGA_GrapplingHook::OnAvatarSet(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
	LoadedMovementProgressCurve = GrapplingData
		? GrapplingData->MovementProgressCurve.LoadSynchronous()
		: nullptr;
	if (GrapplingData
		&& !LoadedMovementProgressCurve
		&& !GrapplingData->MovementProgressCurve.IsNull())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("그래플링 이동 진행 곡선을 미리 불러오지 못해 선형 진행으로 대체합니다. Curve=%s"),
			*GrapplingData->MovementProgressCurve.ToString());
	}

	ALastFPSHero* Hero = ActorInfo
		? Cast<ALastFPSHero>(ActorInfo->AvatarActor.Get())
		: nullptr;
	if (Hero && ensureMsgf(
		Hero->GetGrapplingTargetingComponent(),
		TEXT("그래플링 타기팅 컴포넌트가 없습니다. Hero=%s"),
		*GetNameSafe(Hero)))
	{
		Hero->GetGrapplingTargetingComponent()->ConfigureTargeting(GrapplingData.Get());
	}
}

void UGA_GrapplingHook::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!GrapplingData || !Hero || !Hero->IsAlive()
		|| Hero->GetCombatState() != EMMCombatState::Idle)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_GrapplingHook activation failed: Data=%s Hero=%s"),
			*GetNameSafe(GrapplingData),
			*GetNameSafe(Hero));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ULastFPSGrapplingTargetingComponent* TargetingComponent =
		Hero->GetGrapplingTargetingComponent();
	if (!ensureMsgf(
		TargetingComponent,
		TEXT("그래플링 활성화에 필요한 타기팅 컴포넌트가 없습니다. Hero=%s"),
		*GetNameSafe(Hero)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FHitResult TargetHit;
	if (!TargetingComponent->ResolveGrappleTarget(*GrapplingData, TargetHit))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	GrappleAnchor = TargetHit.ImpactPoint + TargetHit.ImpactNormal * GrapplingData->AnchorSurfaceOffset;
	const FVector ToAnchor = GrappleAnchor - Hero->GetActorLocation();
	if (ToAnchor.SizeSquared() <= KINDA_SMALL_NUMBER || GrapplingData->PullSpeed <= KINDA_SMALL_NUMBER)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	float CapsuleHalfHeight = 0.f;
	if (const UCapsuleComponent* CapsuleComp = Hero->GetCapsuleComponent())
	{
		CapsuleHalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
	}

	// 훅 건 위치(GrappleAnchor)에 캐릭터의 발바닥이 안착하도록 캡슐 반높이를 Z 오프셋으로 반영합니다.
	const FVector PullDestination = GrappleAnchor
		+ FVector::UpVector * (CapsuleHalfHeight + GrapplingData->ArrivalHeightOffset)
		+ TargetHit.ImpactNormal * GrapplingData->ArrivalSurfaceClearance;
	const float PullDistance = FVector::Distance(Hero->GetActorLocation(), PullDestination);
	if (PullDistance <= KINDA_SMALL_NUMBER)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	GrapplePullDestination = PullDestination;
	GrappleSurfaceNormal = TargetHit.ImpactNormal;
	GrappleTargetComponent = TargetHit.GetComponent();
	bPullStarted = false;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Hero->SetWantsToSprint(false);
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayTagContainer SprintTags;
		SprintTags.AddTag(LastFPSGameplayTags::Input_Sprint);
		ASC->CancelAbilities(&SprintTags);
	}
	StartHookFlight(*Hero);
	if (!IsActive())
	{
		return;
	}

	DrawDebugLine(ActorInfo, Hero->GetActorLocation(), GrappleAnchor);
	DrawDebugSphere(ActorInfo, GrappleAnchor, 12.f);
	DrawDebugSphere(ActorInfo, PullDestination, 20.f);
}

void UGA_GrapplingHook::StartHookFlight(ALastFPSHero& Hero)
{
	if (ULastFPSGrapplingAnimationComponent* AnimationComponent =
		Hero.GetGrapplingAnimationComponent())
	{
		AnimationComponent->BeginHookFlight(
			this,
			GrappleAnchor,
			GrapplingData->HookFlightDuration);
		bGrapplingAnimationStateStarted = true;
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("그래플링 훅 비행 애니메이션 상태 시작 실패: Hero=%s, 원인=GrapplingAnimationComponent가 없습니다."),
			*GetNameSafe(&Hero));
	}

	StartWireGameplayCue(Hero, GrappleSurfaceNormal);
	if (GrapplingData->HookFlightDuration <= KINDA_SMALL_NUMBER)
	{
		OnHookAttached();
		return;
	}

	HookFlightTask = UAbilityTask_WaitDelay::WaitDelay(this, GrapplingData->HookFlightDuration);
	if (!HookFlightTask)
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			true);
		return;
	}

	HookFlightTask->OnFinish.AddDynamic(this, &UGA_GrapplingHook::OnHookAttached);
	HookFlightTask->ReadyForActivation();
}

void UGA_GrapplingHook::OnHookAttached()
{
	HookFlightTask = nullptr;
	if (bHookAttached || !IsActive() || !GrapplingData)
	{
		return;
	}
	if (GrappleTargetComponent.IsStale())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GA_GrapplingHook attachment failed: Hero=%s, reason=target component was destroyed during hook flight."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			true);
		return;
	}

	ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!Hero)
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			true);
		return;
	}

	bHookAttached = true;
	if (GrapplingData->AttachDelayDuration <= KINDA_SMALL_NUMBER)
	{
		StartAttachedPull(*Hero);
		return;
	}

	AttachDelayTask = UAbilityTask_WaitDelay::WaitDelay(
		this,
		GrapplingData->AttachDelayDuration);
	if (!AttachDelayTask)
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			true);
		return;
	}

	AttachDelayTask->OnFinish.AddDynamic(this, &UGA_GrapplingHook::OnAttachDelayFinished);
	AttachDelayTask->ReadyForActivation();
}

void UGA_GrapplingHook::OnAttachDelayFinished()
{
	AttachDelayTask = nullptr;
	if (!IsActive() || !GrapplingData)
	{
		return;
	}

	ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!Hero)
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			true);
		return;
	}

	StartAttachedPull(*Hero);
}

void UGA_GrapplingHook::StartAttachedPull(ALastFPSHero& Hero)
{
	if (ULastFPSGrapplingAnimationComponent* AnimationComponent =
		Hero.GetGrapplingAnimationComponent())
	{
		AnimationComponent->BeginPulling(this, GrappleAnchor, GrapplingData->IKSettings);
		bGrapplingAnimationStateStarted = true;
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("GA_GrapplingHook animation state start failed: Hero=%s, reason=GrapplingAnimationComponent is missing."),
			*GetNameSafe(&Hero));
	}
	StartGrapplePull();
}

void UGA_GrapplingHook::StartGrapplePull()
{
	if (bPullStarted || !GrapplingData)
	{
		return;
	}

	ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!Hero)
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			true);
		return;
	}
	const float PullDuration = FVector::Distance(Hero->GetActorLocation(), GrapplePullDestination)
		/ GrapplingData->PullSpeed;
	if (PullDuration <= KINDA_SMALL_NUMBER)
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			false);
		return;
	}

	bPullStarted = true;
	const ERootMotionFinishVelocityMode FinishMode = ResolveFinishVelocityMode(
		GrapplingData->FinishVelocityMode);
	UPrimitiveComponent* TargetComponent = GrappleTargetComponent.Get();
	if (!IsValid(TargetComponent))
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			true);
		return;
	}

	const FVector TargetRelativeDestination =
		TargetComponent->GetComponentTransform().InverseTransformPosition(GrapplePullDestination);
	GrappleMovementTask =
		UAbilityTask_ApplyRootMotionMoveToActorForce::ApplyRootMotionMoveToComponentForce(
		this,
		TEXT("GrapplingHookPull"),
		TargetComponent,
		TargetRelativeDestination,
		FVector::ZeroVector,
		ERootMotionMoveToActorTargetOffsetType::AlignToWorldSpace,
		PullDuration,
		nullptr,
		nullptr,
		true,
		MOVE_Flying,
		GrapplingData->bRestrictSpeedToExpected,
		GrapplingData->PathOffsetCurve,
		LoadedMovementProgressCurve,
		FinishMode,
		FVector::ZeroVector,
		GrapplingData->FinishVelocityClamp,
		true,
		FMath::Max(GrapplingData->StopDistance, 0.0f));
	if (!GrappleMovementTask)
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			true);
		return;
	}

	GrappleMovementTask->OnFinished.AddDynamic(
		this,
		&UGA_GrapplingHook::OnGrappleMovementFinished);
	FLastFPSTemporaryCameraEffectOptions CameraEffectOptions;
	CameraEffectOptions.CameraLagSpeed = GrapplingData->PullCameraLagSpeed;
	CameraEffectOptions.MotionBlurAmount = GrapplingData->PullMotionBlurAmount;
	CameraEffectOptions.BlendOutDuration = GrapplingData->PullCameraBlendOutDuration;
	bCameraEffectStarted = Hero->BeginTemporaryCameraEffect(this, CameraEffectOptions);
	GrappleMovementTask->ReadyForActivation();
}

void UGA_GrapplingHook::StartWireGameplayCue(
	ALastFPSHero& Hero,
	const FVector& SurfaceNormal)
{
	if (!GrapplingData || !GrapplingData->WireGameplayCueTag.IsValid())
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Location = GrappleAnchor;
	CueParameters.Normal = SurfaceNormal;
	CueParameters.RawMagnitude = FVector::Distance(Hero.GetActorLocation(), GrappleAnchor);
	CueParameters.Instigator = &Hero;
	CueParameters.EffectCauser = &Hero;
	CueParameters.SourceObject = GrapplingData;
	CueParameters.TargetAttachComponent = Hero.GetRootComponent();
	K2_AddGameplayCueWithParams(GrapplingData->WireGameplayCueTag, CueParameters, true);
	bWireGameplayCueActive = true;
}

void UGA_GrapplingHook::OnGrappleMovementFinished(
	const bool /*bDestinationReached*/,
	const bool /*bTimedOut*/,
	const FVector /*FinalTargetLocation*/)
{
	GrappleMovementTask = nullptr;
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false);
}

void UGA_GrapplingHook::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
	{
		if (bCameraEffectStarted)
		{
			Hero->EndTemporaryCameraEffect(this);
		}
	}
	bCameraEffectStarted = false;
	if (bGrapplingAnimationStateStarted)
	{
		if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
		{
			if (ULastFPSGrapplingAnimationComponent* AnimationComponent =
				Hero->GetGrapplingAnimationComponent())
			{
				AnimationComponent->EndGrappling(this);
			}
		}
		bGrapplingAnimationStateStarted = false;
	}

	if (GrappleMovementTask)
	{
		GrappleMovementTask->EndTask();
		GrappleMovementTask = nullptr;
	}
	if (HookFlightTask)
	{
		HookFlightTask->EndTask();
		HookFlightTask = nullptr;
	}
	if (AttachDelayTask)
	{
		AttachDelayTask->EndTask();
		AttachDelayTask = nullptr;
	}

	if (bWireGameplayCueActive && GrapplingData && GrapplingData->WireGameplayCueTag.IsValid())
	{
		K2_RemoveGameplayCue(GrapplingData->WireGameplayCueTag);
	}
	bWireGameplayCueActive = false;
	GrappleAnchor = FVector::ZeroVector;
	GrapplePullDestination = FVector::ZeroVector;
	GrappleSurfaceNormal = FVector::UpVector;
	GrappleTargetComponent.Reset();
	bPullStarted = false;
	bGrapplingAnimationStateStarted = false;
	bHookAttached = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
