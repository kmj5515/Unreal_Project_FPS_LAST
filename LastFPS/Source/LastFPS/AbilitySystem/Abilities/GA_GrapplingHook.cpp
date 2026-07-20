#include "AbilitySystem/Abilities/GA_GrapplingHook.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Character/Components/LastFPSGrapplingAnimationComponent.h"
#include "Character/LastFPSHero.h"
#include "Components/PrimitiveComponent.h"
#include "Data/Abilities/LastFPSGrapplingHookData.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
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

	FHitResult TargetHit;
	if (!ResolveGrappleTarget(*Hero, TargetHit))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	GrappleAnchor = TargetHit.ImpactPoint + TargetHit.ImpactNormal * GrapplingData->AnchorSurfaceOffset;
	const FVector ToAnchor = GrappleAnchor - Hero->GetActorLocation();
	const float AnchorApproachDistance = ToAnchor.Size() - GrapplingData->StopDistance;
	if (AnchorApproachDistance <= KINDA_SMALL_NUMBER || GrapplingData->PullSpeed <= KINDA_SMALL_NUMBER)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector AnchorApproachLocation = Hero->GetActorLocation()
		+ ToAnchor.GetSafeNormal() * AnchorApproachDistance;
	const FVector PullDestination = AnchorApproachLocation
		+ FVector::UpVector * GrapplingData->ArrivalHeightOffset
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
	GrappleMovementTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		TEXT("GrapplingHookPull"),
		GrapplePullDestination,
		PullDuration,
		true,
		MOVE_Flying,
		GrapplingData->bRestrictSpeedToExpected,
		GrapplingData->PathOffsetCurve,
		FinishMode,
		FVector::ZeroVector,
		GrapplingData->FinishVelocityClamp);
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

	GrappleMovementTask->OnTimedOut.AddDynamic(this, &UGA_GrapplingHook::OnGrappleMovementFinished);
	GrappleMovementTask->OnTimedOutAndDestinationReached.AddDynamic(
		this,
		&UGA_GrapplingHook::OnGrappleMovementFinished);
	FLastFPSTemporaryCameraEffectOptions CameraEffectOptions;
	CameraEffectOptions.CameraLagSpeed = GrapplingData->PullCameraLagSpeed;
	CameraEffectOptions.MotionBlurAmount = GrapplingData->PullMotionBlurAmount;
	CameraEffectOptions.BlendOutDuration = GrapplingData->PullCameraBlendOutDuration;
	bCameraEffectStarted = Hero->BeginTemporaryCameraEffect(this, CameraEffectOptions);
	GrappleMovementTask->ReadyForActivation();
}

bool UGA_GrapplingHook::ResolveGrappleTarget(const ALastFPSHero& Hero, FHitResult& OutHit) const
{
	if (!GrapplingData || !Hero.GetWorld())
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Hero.GetActorEyesViewPoint(ViewLocation, ViewRotation);
	if (const AController* Controller = Hero.GetController())
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * GrapplingData->MaximumDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GrapplingHookTrace), false, &Hero);
	QueryParams.AddIgnoredActor(&Hero);
	if (!Hero.GetWorld()->LineTraceSingleByChannel(
		OutHit,
		ViewLocation,
		TraceEnd,
		GrapplingData->TraceChannel,
		QueryParams))
	{
		return false;
	}

	const float TargetDistance = FVector::Distance(Hero.GetActorLocation(), OutHit.ImpactPoint);
	if (TargetDistance < GrapplingData->MinimumDistance
		|| TargetDistance > GrapplingData->MaximumDistance)
	{
		return false;
	}

	const UPrimitiveComponent* HitComponent = OutHit.GetComponent();
	return !GrapplingData->bRequireStaticSurface
		|| (HitComponent && HitComponent->GetMobility() == EComponentMobility::Static);
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

void UGA_GrapplingHook::OnGrappleMovementFinished()
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
