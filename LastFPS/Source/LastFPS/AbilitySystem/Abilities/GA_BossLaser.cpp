#include "AbilitySystem/Abilities/GA_BossLaser.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "Character/AI/LastFPSEnemyAIController.h"
#include "Character/Components/LastFPSCombatAimComponent.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSEnemyCharacter.h"
#include "CollisionShape.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Enemies/LastFPSBossLaserAttackData.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "Utility/LastFPSCombatAffiliation.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "Utility/LastFPSTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSBossLaser, Log, All);

UGA_BossLaser::UGA_BossLaser()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Enemy_Boss_Laser);
	SetAssetTags(Tags);
}

bool UGA_BossLaser::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ALastFPSEnemyCharacter* Enemy =
		ActorInfo ? Cast<ALastFPSEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Enemy
		|| !Enemy->IsAlive()
		|| !Enemy->GetCombatAimComponent()
		|| !AttackData
		|| AttackData->BeamDuration <= 0.f
		|| AttackData->DamageInterval <= 0.f
		|| AttackData->BeamRange <= 0.f
		|| AttackData->BeamRadius <= 0.f
		|| AttackData->EffectsOnHit.IsEmpty())
	{
		return false;
	}

	const AActor* TargetActor = ResolveCombatTargetForSource(*Enemy);
	return TargetActor && IsTargetWithinActivationEnvelope(*Enemy, *TargetActor);
}

void UGA_BossLaser::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bEndingAbility = false;
	bHasTargetAim = false;
	bTrackingFocusActive = false;
	bChargeGameplayCueActive = false;
	bPreviewGameplayCueActive = false;
	bBeamGameplayCueActive = false;
	CurrentTarget.Reset();
	LockedAimLocation = FVector::ZeroVector;
	LockedBeamDirection = FVector::ForwardVector;
	CurrentBeamStart = FVector::ZeroVector;
	CurrentBeamEnd = FVector::ZeroVector;
	SourceEnemy = Cast<ALastFPSEnemyCharacter>(GetAvatarActorFromActorInfo());
	AimComponent = SourceEnemy ? SourceEnemy->GetCombatAimComponent() : nullptr;

	if (!SourceEnemy || !SourceEnemy->HasAuthority() || !SourceEnemy->IsAlive()
		|| !AimComponent || !AttackData || AttackData->EffectsOnHit.IsEmpty())
	{
		UE_LOG(LogLastFPSBossLaser, Error,
			TEXT("보스 레이저 활성화 실패: Source=%s, AimComponent=%s, AttackData=%s, 원인=서버 실행 조건 또는 필수 설정이 유효하지 않습니다."),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(AimComponent),
			*GetNameSafe(AttackData));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* InitialTarget = ResolveCombatTarget();
	if (!InitialTarget || !IsTargetWithinActivationEnvelope(*SourceEnemy, *InitialTarget))
	{
		UE_LOG(LogLastFPSBossLaser, Warning,
			TEXT("보스 레이저 활성화 실패: Source=%s, Target=%s, 원인=대상이 없거나 최소 거리·전방 범위 조건을 만족하지 않습니다."),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(InitialTarget));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentTarget = InitialTarget;
	StartTracking();
}

void UGA_BossLaser::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	bEndingAbility = true;
	ClearRuntimeTimers();
	ClearTrackingFocus();

	if (AttackData)
	{
		StopGameplayCue(AttackData->ChargeGameplayCueTag, bChargeGameplayCueActive);
		StopGameplayCue(AttackData->PreviewGameplayCueTag, bPreviewGameplayCueActive);
		StopGameplayCue(AttackData->BeamGameplayCueTag, bBeamGameplayCueActive);
	}

	if (AimComponent)
	{
		AimComponent->EndFiring(this);
		AimComponent->ClearAimTarget(this);
	}

	CurrentTarget.Reset();
	LockedAimLocation = FVector::ZeroVector;
	AimComponent = nullptr;
	SourceEnemy = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_BossLaser::StartTracking()
{
	if (!RefreshTargetAim())
	{
		FinishCurrentAbility(true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !AttackData)
	{
		FinishCurrentAbility(true);
		return;
	}

	if (AttackData->PreviewGameplayCueTag.IsValid())
	{
		UpdateTrackingPreview();
	}
	else
	{
		CurrentBeamStart = ResolveMuzzleLocation();
		const AActor* ChargeTarget = CurrentTarget.Get();
		FVector ChargeDirection = ChargeTarget
			? ResolveStableAimDirection(CurrentBeamStart, ResolveTargetLocation(*ChargeTarget))
			: SourceEnemy->GetActorForwardVector();
		if (ChargeDirection.IsNearlyZero())
		{
			ChargeDirection = SourceEnemy->GetActorForwardVector();
		}
		FGameplayCueParameters ChargeCueParameters;
		ChargeCueParameters.Location = CurrentBeamStart;
		ChargeCueParameters.Normal = ChargeDirection;
		ChargeCueParameters.RawMagnitude = ChargeTarget
			? FVector::Distance(CurrentBeamStart, ResolveTargetLocation(*ChargeTarget))
			: 0.f;
		StartGameplayCue(
			AttackData->ChargeGameplayCueTag,
			bChargeGameplayCueActive,
			&ChargeCueParameters);
	}
	if (AttackData->TrackingDuration <= 0.f)
	{
		FinishTracking();
		return;
	}

	bTrackingFocusActive = true;
	UpdateTrackingFocus();
	World->GetTimerManager().SetTimer(
		TrackingUpdateTimerHandle,
		this,
		&UGA_BossLaser::UpdateTracking,
		FMath::Max(AttackData->AimUpdateInterval, 0.01f),
		true);
	World->GetTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&UGA_BossLaser::FinishTracking,
		AttackData->TrackingDuration,
		false);
}

void UGA_BossLaser::UpdateTracking()
{
	if (RefreshTargetAim())
	{
		UpdateTrackingFocus();
		UpdateTrackingPreview();
	}
}

void UGA_BossLaser::UpdateTrackingPreview()
{
	if (bEndingAbility || !AttackData || !SourceEnemy || !AttackData->PreviewGameplayCueTag.IsValid())
	{
		return;
	}

	const AActor* TargetActor = CurrentTarget.Get();
	if (!IsValid(TargetActor))
	{
		return;
	}

	CurrentBeamStart = ResolveMuzzleLocation();
	FVector PreviewDirection = ResolveStableAimDirection(
		CurrentBeamStart,
		ResolveTargetLocation(*TargetActor));
	if (PreviewDirection.IsNearlyZero())
	{
		PreviewDirection = SourceEnemy->GetActorForwardVector();
	}

	CurrentBeamEnd = CurrentBeamStart
		+ PreviewDirection * FMath::Max(AttackData->BeamRange, 1.f);
	ClipBeamEndToWorld();
	PushPreviewGameplayCue(PreviewDirection);
}

void UGA_BossLaser::PushPreviewGameplayCue(const FVector& PreviewDirection)
{
	if (!AttackData || !AttackData->PreviewGameplayCueTag.IsValid())
	{
		return;
	}

	FGameplayCueParameters PreviewCueParameters;
	PreviewCueParameters.Location = CurrentBeamStart;
	PreviewCueParameters.Normal = PreviewDirection;
	PreviewCueParameters.RawMagnitude = FVector::Distance(CurrentBeamStart, CurrentBeamEnd);

	if (!bPreviewGameplayCueActive)
	{
		StartGameplayCue(
			AttackData->PreviewGameplayCueTag,
			bPreviewGameplayCueActive,
			&PreviewCueParameters);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ExecuteGameplayCue(AttackData->PreviewGameplayCueTag, PreviewCueParameters);
	}
}

void UGA_BossLaser::FinishTracking()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TrackingUpdateTimerHandle);
	}
	ClearTrackingFocus();

	if (!LockBeamPathFromCurrentTarget())
	{
		FinishCurrentAbility(true);
		return;
	}
	PushPreviewGameplayCue(LockedBeamDirection);

	if (AttackData->LockDuration <= 0.f)
	{
		StartBeam();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PhaseTimerHandle,
			this,
			&UGA_BossLaser::StartBeam,
			AttackData->LockDuration,
			false);
	}
	else
	{
		FinishCurrentAbility(true);
	}
}

void UGA_BossLaser::StartBeam()
{
	if (bEndingAbility || !AttackData || !SourceEnemy || !SourceEnemy->IsAlive() || !AimComponent)
	{
		FinishCurrentAbility(true);
		return;
	}

	const AActor* LockedTarget = CurrentTarget.Get();
	if (!IsValid(LockedTarget) || !IsTargetWithinActivationEnvelope(*SourceEnemy, *LockedTarget))
	{
		UE_LOG(LogLastFPSBossLaser, Log,
			TEXT("보스 레이저 발사 취소: Source=%s, Target=%s, 원인=잠금 중 대상이 최소 거리 또는 전방 범위를 벗어났습니다."),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(LockedTarget));
		FinishCurrentAbility(true);
		return;
	}

	StopGameplayCue(AttackData->ChargeGameplayCueTag, bChargeGameplayCueActive);
	StopGameplayCue(AttackData->PreviewGameplayCueTag, bPreviewGameplayCueActive);
	CurrentBeamStart = ResolveMuzzleLocation();
	LockedBeamDirection = ResolveStableAimDirection(CurrentBeamStart, LockedAimLocation);
	if (LockedBeamDirection.IsNearlyZero())
	{
		LockedBeamDirection = SourceEnemy->GetActorForwardVector();
	}
	CurrentBeamEnd = CurrentBeamStart
		+ LockedBeamDirection * FMath::Max(AttackData->BeamRange, 1.f);
	ClipBeamEndToWorld();

	FGameplayCueParameters BeamCueParameters;
	BeamCueParameters.Location = CurrentBeamStart;
	BeamCueParameters.Normal = LockedBeamDirection;
	// Gameplay Cue가 Niagara의 User.Length에 연결할 실제 차단 후 레이저 길이다.
	BeamCueParameters.RawMagnitude = FVector::Distance(CurrentBeamStart, CurrentBeamEnd);
	StartGameplayCue(
		AttackData->BeamGameplayCueTag,
		bBeamGameplayCueActive,
		&BeamCueParameters);
	AimComponent->BeginFiring(this);
	ApplyBeamPulse();
	if (bEndingAbility)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		FinishCurrentAbility(true);
		return;
	}

	World->GetTimerManager().SetTimer(
		DamageTimerHandle,
		this,
		&UGA_BossLaser::ApplyBeamPulse,
		FMath::Max(AttackData->DamageInterval, 0.01f),
		true);
	World->GetTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&UGA_BossLaser::FinishBeam,
		FMath::Max(AttackData->BeamDuration, 0.01f),
		false);
}

void UGA_BossLaser::ApplyBeamPulse()
{
	if (bEndingAbility || !AttackData || !SourceEnemy || !SourceEnemy->IsAlive() || !AimComponent)
	{
		FinishCurrentAbility(true);
		return;
	}

	if (AttackData->bTrackDuringBeam && !LockBeamPathFromCurrentTarget())
	{
		if (AttackData->bCancelIfTargetLost)
		{
			FinishCurrentAbility(true);
		}
		return;
	}

	CurrentBeamStart = ResolveMuzzleLocation();
	CurrentBeamEnd = CurrentBeamStart
		+ LockedBeamDirection * FMath::Max(AttackData->BeamRange, 1.f);
	ClipBeamEndToWorld();

#if ENABLE_DRAW_DEBUG
	if (ShouldDrawDebug())
	{
		// 실제 서버 판정과 동일한 시작점, 차단된 끝점, 반경을 표시한다.
		DrawDebugLine(GetCurrentActorInfo(), CurrentBeamStart, CurrentBeamEnd);
		DrawDebugSphere(GetCurrentActorInfo(), CurrentBeamStart, AttackData->BeamRadius);
		DrawDebugSphere(GetCurrentActorInfo(), CurrentBeamEnd, AttackData->BeamRadius);
	}
#endif

	UWorld* World = GetWorld();
	if (!World)
	{
		FinishCurrentAbility(true);
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossLaserDamage), false, SourceEnemy.Get());
	QueryParams.AddIgnoredActor(SourceEnemy.Get());
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> HitResults;
	World->SweepMultiByObjectType(
		HitResults,
		CurrentBeamStart,
		CurrentBeamEnd,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(FMath::Max(AttackData->BeamRadius, 1.f)),
		QueryParams);
	HitResults.Sort([](const FHitResult& Left, const FHitResult& Right)
	{
		return Left.Distance < Right.Distance;
	});

	TSet<TWeakObjectPtr<AActor>> ProcessedActors;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) || ProcessedActors.Contains(TargetActor)
			|| !DoesTargetPassConditions(*TargetActor))
		{
			continue;
		}

		ProcessedActors.Add(TargetActor);
		if (ApplyEffectsToTarget(*TargetActor) && !AttackData->bHitMultipleTargets)
		{
			break;
		}
	}
}

void UGA_BossLaser::FinishBeam()
{
	if (bEndingAbility || !AttackData)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DamageTimerHandle);
	}
	StopGameplayCue(AttackData->BeamGameplayCueTag, bBeamGameplayCueActive);
	if (AimComponent)
	{
		AimComponent->EndFiring(this);
		AimComponent->ClearAimTarget(this);
	}

	if (AttackData->RecoveryDuration <= 0.f)
	{
		FinishRecovery();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PhaseTimerHandle,
			this,
			&UGA_BossLaser::FinishRecovery,
			AttackData->RecoveryDuration,
			false);
	}
	else
	{
		FinishCurrentAbility(true);
	}
}

void UGA_BossLaser::FinishRecovery()
{
	FinishCurrentAbility(false);
}

void UGA_BossLaser::FinishCurrentAbility(const bool bWasCancelled)
{
	if (bEndingAbility || !IsActive())
	{
		return;
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled);
}

void UGA_BossLaser::ClearRuntimeTimers()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(TrackingUpdateTimerHandle);
		TimerManager.ClearTimer(PhaseTimerHandle);
		TimerManager.ClearTimer(DamageTimerHandle);
	}
}

void UGA_BossLaser::UpdateTrackingFocus()
{
	if (!bTrackingFocusActive || !SourceEnemy)
	{
		return;
	}

	AAIController* OwningAIController = Cast<AAIController>(SourceEnemy->GetController());
	AActor* TargetActor = CurrentTarget.Get();
	if (OwningAIController && IsValid(TargetActor))
	{
		// 추적 구간만 Gameplay Focus를 소유하여 다른 AI 회전 요청과 충돌하지 않게 한다.
		OwningAIController->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	}
}

void UGA_BossLaser::ClearTrackingFocus()
{
	if (!bTrackingFocusActive)
	{
		return;
	}

	bTrackingFocusActive = false;
	if (SourceEnemy)
	{
		if (AAIController* OwningAIController = Cast<AAIController>(SourceEnemy->GetController()))
		{
			OwningAIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}

void UGA_BossLaser::StartGameplayCue(
	const FGameplayTag& CueTag,
	bool& bCueActive,
	const FGameplayCueParameters* CueParameters)
{
	if (bCueActive || !CueTag.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (CueParameters)
		{
			ASC->AddGameplayCue(CueTag, *CueParameters);
		}
		else
		{
			ASC->AddGameplayCue(CueTag);
		}
		bCueActive = true;
	}
}

void UGA_BossLaser::StopGameplayCue(const FGameplayTag& CueTag, bool& bCueActive)
{
	if (!bCueActive)
	{
		return;
	}

	bCueActive = false;
	if (CueTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveGameplayCue(CueTag);
		}
	}
}

AActor* UGA_BossLaser::ResolveCombatTarget() const
{
	if (!SourceEnemy)
	{
		return nullptr;
	}

	return ResolveCombatTargetForSource(*SourceEnemy);
}

AActor* UGA_BossLaser::ResolveCombatTargetForSource(const ALastFPSEnemyCharacter& Enemy) const
{

	AActor* TargetActor = nullptr;
	const AAIController* OwningAIController = Cast<AAIController>(Enemy.GetController());
	if (const ALastFPSEnemyAIController* EnemyController =
		Cast<ALastFPSEnemyAIController>(OwningAIController))
	{
		TargetActor = EnemyController->GetCombatTargetActor();
		if (!TargetActor)
		{
			TargetActor = EnemyController->GetFocusActor();
		}
	}
	else if (OwningAIController)
	{
		TargetActor = OwningAIController->GetFocusActor();
	}

	if (!IsValid(TargetActor))
	{
		return nullptr;
	}
	if (AttackData && AttackData->bRequireLineOfSight
		&& OwningAIController && !OwningAIController->LineOfSightTo(TargetActor))
	{
		return nullptr;
	}
	if (const ALastFPSCharacterBase* TargetCharacter = Cast<ALastFPSCharacterBase>(TargetActor);
		TargetCharacter && !TargetCharacter->IsAlive())
	{
		return nullptr;
	}
	return TargetActor;
}

bool UGA_BossLaser::IsTargetWithinActivationEnvelope(
	const ALastFPSEnemyCharacter& Enemy,
	const AActor& TargetActor) const
{
	if (!AttackData)
	{
		return false;
	}

	// 애니메이션에 따라 움직이는 총구가 아니라 보스 루트를 기준으로 선택 조건을 안정화한다.
	const FVector ToTarget2D = FVector::VectorPlaneProject(
		TargetActor.GetActorLocation() - Enemy.GetActorLocation(),
		FVector::UpVector);
	const float MinimumDistance = FMath::Max(AttackData->MinimumActivationDistance, 0.f);
	if (ToTarget2D.SizeSquared() < FMath::Square(MinimumDistance))
	{
		return false;
	}

	const FVector TargetDirection2D = ToTarget2D.GetSafeNormal();
	const FVector EnemyForward2D = FVector::VectorPlaneProject(
		Enemy.GetActorForwardVector(),
		FVector::UpVector).GetSafeNormal();
	if (TargetDirection2D.IsNearlyZero() || EnemyForward2D.IsNearlyZero())
	{
		return false;
	}

	const float MinimumForwardDot = FMath::Clamp(
		AttackData->MinimumActivationForwardDot,
		-1.f,
		1.f);
	return FVector::DotProduct(EnemyForward2D, TargetDirection2D) >= MinimumForwardDot;
}

FVector UGA_BossLaser::ResolveTargetLocation(const AActor& TargetActor) const
{
	return TargetActor.GetActorLocation()
		+ FVector::UpVector * (AttackData ? AttackData->TargetAimHeight : 0.f);
}

FVector UGA_BossLaser::ResolveMuzzleLocation() const
{
	if (!SourceEnemy || !AttackData)
	{
		return FVector::ZeroVector;
	}

	const USkeletalMeshComponent* Mesh = SourceEnemy->GetMesh();
	if (Mesh && !AttackData->MuzzleSocketName.IsNone()
		&& Mesh->DoesSocketExist(AttackData->MuzzleSocketName))
	{
		return Mesh->GetSocketLocation(AttackData->MuzzleSocketName);
	}

	return SourceEnemy->GetActorTransform().TransformPosition(AttackData->FallbackMuzzleOffset);
}

FVector UGA_BossLaser::ResolveStableAimDirection(
	const FVector& MuzzleLocation,
	const FVector& TargetLocation) const
{
	const FVector ToTarget = TargetLocation - MuzzleLocation;
	const FVector HorizontalOffset(ToTarget.X, ToTarget.Y, 0.f);
	const float HorizontalDistance = HorizontalOffset.Size();
	const float MinimumHorizontalDistance = AttackData
		? FMath::Max(AttackData->MinimumHorizontalAimDistance, 1.f)
		: 1.f;

	FVector SourceForward = SourceEnemy ? SourceEnemy->GetActorForwardVector() : FVector::ForwardVector;
	SourceForward.Z = 0.f;
	SourceForward = SourceForward.GetSafeNormal();
	if (SourceForward.IsNearlyZero())
	{
		SourceForward = FVector::ForwardVector;
	}

	const FVector TargetHorizontalDirection = HorizontalOffset.GetSafeNormal();
	const float TargetDirectionWeight = FMath::Clamp(
		HorizontalDistance / MinimumHorizontalDistance,
		0.f,
		1.f);
	FVector HorizontalDirection = FMath::Lerp(
		SourceForward,
		TargetHorizontalDirection.IsNearlyZero() ? SourceForward : TargetHorizontalDirection,
		TargetDirectionWeight).GetSafeNormal();
	if (HorizontalDirection.IsNearlyZero())
	{
		HorizontalDirection = SourceForward;
	}

	// 수직 발산은 최대 피치가 제한하므로 실제 수평 거리로 상하 각도를 계산한다.
	const float PitchReferenceDistance = FMath::Max(HorizontalDistance, 1.f);
	const float RawPitch = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Z, PitchReferenceDistance));
	const float MaximumUpwardPitch = AttackData
		? FMath::Clamp(AttackData->MaximumUpwardAimPitch, 0.f, 89.f)
		: 89.f;
	const float MaximumDownwardPitch = AttackData
		? FMath::Clamp(AttackData->MaximumDownwardAimPitch, 0.f, 89.f)
		: 89.f;

	FRotator AimRotation = HorizontalDirection.Rotation();
	AimRotation.Pitch = FMath::Clamp(RawPitch, -MaximumDownwardPitch, MaximumUpwardPitch);
	return AimRotation.Vector();
}

bool UGA_BossLaser::RefreshTargetAim()
{
	if (bEndingAbility || !SourceEnemy || !SourceEnemy->IsAlive() || !AimComponent || !AttackData)
	{
		return false;
	}

	AActor* TargetActor = ResolveCombatTarget();
	if (!TargetActor)
	{
		if (AttackData->bCancelIfTargetLost)
		{
			UE_LOG(LogLastFPSBossLaser, Warning,
				TEXT("보스 레이저 취소: Source=%s, 원인=추적 중 전투 대상을 잃었습니다."),
				*GetNameSafe(SourceEnemy));
			FinishCurrentAbility(true);
		}
		return false;
	}
	if (!IsTargetWithinActivationEnvelope(*SourceEnemy, *TargetActor))
	{
		UE_LOG(LogLastFPSBossLaser, Log,
			TEXT("보스 레이저 추적 취소: Source=%s, Target=%s, 원인=대상이 최소 거리 또는 전방 범위를 벗어났습니다."),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(TargetActor));
		FinishCurrentAbility(true);
		return false;
	}

	CurrentTarget = TargetActor;
	bHasTargetAim = true;
	AimComponent->SetAimTarget(this, ResolveTargetLocation(*TargetActor));
	return true;
}

bool UGA_BossLaser::LockBeamPathFromCurrentTarget()
{
	AActor* TargetActor = ResolveCombatTarget();
	if (TargetActor)
	{
		CurrentTarget = TargetActor;
		bHasTargetAim = true;
	}
	else if (AttackData && AttackData->bCancelIfTargetLost)
	{
		return false;
	}

	TargetActor = CurrentTarget.Get();
	if (!IsValid(TargetActor) || !bHasTargetAim || !AttackData)
	{
		return false;
	}
	if (!SourceEnemy || !IsTargetWithinActivationEnvelope(*SourceEnemy, *TargetActor))
	{
		return false;
	}

	CurrentBeamStart = ResolveMuzzleLocation();
	const FVector TargetLocation = ResolveTargetLocation(*TargetActor);
	LockedAimLocation = TargetLocation;
	LockedBeamDirection = ResolveStableAimDirection(
		CurrentBeamStart,
		TargetLocation);
	if (LockedBeamDirection.IsNearlyZero())
	{
		LockedBeamDirection = SourceEnemy ? SourceEnemy->GetActorForwardVector() : FVector::ForwardVector;
	}

	// ABP에는 발사 판정 끝점이 아니라 잠근 실제 타겟 위치를 유지한다.
	AimComponent->SetAimTarget(this, LockedAimLocation);

	CurrentBeamEnd = CurrentBeamStart + LockedBeamDirection * FMath::Max(AttackData->BeamRange, 1.f);
	ClipBeamEndToWorld();
	return true;
}

void UGA_BossLaser::ClipBeamEndToWorld()
{
	UWorld* World = GetWorld();
	if (!World || !SourceEnemy || !AttackData)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossLaserBlocking), false, SourceEnemy.Get());
	QueryParams.AddIgnoredActor(SourceEnemy.Get());
	const FCollisionObjectQueryParams ObjectParams(AttackData->BlockingObjectTypes);
	if (!ObjectParams.IsValid())
	{
		return;
	}

	FHitResult BlockingHit;
	if (World->LineTraceSingleByObjectType(
		BlockingHit,
		CurrentBeamStart,
		CurrentBeamEnd,
		ObjectParams,
		QueryParams))
	{
		CurrentBeamEnd = BlockingHit.ImpactPoint;
	}
}

bool UGA_BossLaser::DoesTargetPassConditions(AActor& TargetActor) const
{
	if (!AttackData || &TargetActor == SourceEnemy.Get())
	{
		return false;
	}
	if (const ALastFPSCharacterBase* TargetCharacter = Cast<ALastFPSCharacterBase>(&TargetActor);
		TargetCharacter && !TargetCharacter->IsAlive())
	{
		return false;
	}
	if (AttackData->bIgnoreFriendlyTargets
		&& LastFPSCombatAffiliation::AreFriendlyActors(SourceEnemy.Get(), &TargetActor))
	{
		return false;
	}

	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(&TargetActor);
	UAbilitySystemComponent* TargetASC =
		AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC)
	{
		return false;
	}

	FGameplayTagContainer OwnedTags;
	TargetASC->GetOwnedGameplayTags(OwnedTags);
	return (AttackData->RequiredTargetTags.IsEmpty()
			|| OwnedTags.HasAll(AttackData->RequiredTargetTags))
		&& (AttackData->BlockedTargetTags.IsEmpty()
			|| !OwnedTags.HasAny(AttackData->BlockedTargetTags));
}

bool UGA_BossLaser::ApplyEffectsToTarget(AActor& TargetActor) const
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(&TargetActor);
	UAbilitySystemComponent* TargetASC =
		AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC || !AttackData)
	{
		return false;
	}

	bool bAppliedAnyEffect = false;
	for (const TSubclassOf<UGameplayEffect>& EffectClass : AttackData->EffectsOnHit)
	{
		bAppliedAnyEffect |= ApplyEffectToTarget(TargetActor, *TargetASC, EffectClass);
	}
	return bAppliedAnyEffect;
}

bool UGA_BossLaser::ApplyEffectToTarget(
	AActor& TargetActor,
	UAbilitySystemComponent& TargetASC,
	const TSubclassOf<UGameplayEffect> EffectClass) const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC || !SourceEnemy || !AttackData || !EffectClass)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(AttackData);
	Context.AddInstigator(SourceEnemy.Get(), SourceEnemy.Get());
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (!Spec.IsValid() || !Spec.Data.IsValid())
	{
		UE_LOG(LogLastFPSBossLaser, Warning,
			TEXT("보스 레이저 효과 생성 실패: Source=%s, Target=%s, Effect=%s"),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(&TargetActor),
			*GetNameSafe(EffectClass));
		return false;
	}

	if (LastFPSDamage::IsDamageGameplayEffect(EffectClass))
	{
		LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), AttackData->DamageRange);
	}

	return TargetASC.ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()).WasSuccessfullyApplied();
}
