#include "AbilitySystem/Abilities/GA_BossBurstShoot.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Character/AI/LastFPSEnemyAIController.h"
#include "Character/Components/LastFPSCombatAimComponent.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSEnemyCharacter.h"
#include "Data/Enemies/LastFPSBossBurstAttackData.h"
#include "Data/Projectiles/LastFPSAbilityProjectileData.h"
#include "Engine/World.h"
#include "Projectiles/LastFPSProjectile.h"
#include "Projectiles/LastFPSProjectileLaunchUtility.h"
#include "TimerManager.h"
#include "Utility/LastFPSTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSBossBurstShoot, Log, All);

UGA_BossBurstShoot::UGA_BossBurstShoot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Enemy_Boss_BurstShoot);
	SetAssetTags(Tags);
}

bool UGA_BossBurstShoot::CanActivateAbility(
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
	return Enemy
		&& Enemy->IsAlive()
		&& Enemy->GetCombatAimComponent()
		&& AttackData
		&& AttackData->ProjectileData
		&& AttackData->ProjectileData->ProjectileClass
		&& AttackData->BurstCount > 0;
}

void UGA_BossBurstShoot::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bEndingAbility = false;
	bHasAimLocation = false;
	bTrackingFocusActive = false;
	bChargeGameplayCueActive = false;
	FiredShotCount = 0;
	LockedAimLocation = FVector::ZeroVector;
	CurrentTarget.Reset();
	SourceEnemy = Cast<ALastFPSEnemyCharacter>(GetAvatarActorFromActorInfo());
	AimComponent = SourceEnemy ? SourceEnemy->GetCombatAimComponent() : nullptr;

	if (!SourceEnemy || !SourceEnemy->HasAuthority() || !SourceEnemy->IsAlive()
		|| !AimComponent || !AttackData || !AttackData->ProjectileData
		|| !AttackData->ProjectileData->ProjectileClass || AttackData->BurstCount <= 0)
	{
		UE_LOG(LogLastFPSBossBurstShoot, Error,
			TEXT("보스 버스트 공격 활성화 실패: Source=%s, AimComponent=%s, AttackData=%s, ProjectileData=%s"),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(AimComponent),
			*GetNameSafe(AttackData),
			*GetNameSafe(AttackData ? AttackData->ProjectileData : nullptr));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* InitialTarget = ResolveCombatTarget();
	if (!InitialTarget)
	{
		UE_LOG(LogLastFPSBossBurstShoot, Warning,
			TEXT("보스 버스트 공격 활성화 실패: Source=%s, 원인=유효한 전투 타겟이 없습니다."),
			*GetNameSafe(SourceEnemy));
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

void UGA_BossBurstShoot::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	bEndingAbility = true;
	ClearRuntimeTimers();
	ClearTrackingFocus();
	StopChargeGameplayCue();

	if (AimComponent)
	{
		AimComponent->EndFiring(this);
		AimComponent->ClearAimTarget(this);
	}

	CurrentTarget.Reset();
	AimComponent = nullptr;
	SourceEnemy = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_BossBurstShoot::StartTracking()
{
	const bool bAimUpdated = RefreshAimFromTarget();
	if (bEndingAbility)
	{
		return;
	}
	if (!bAimUpdated && !bHasAimLocation)
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

	if (AttackData->TrackingDuration + AttackData->LockDuration > 0.f)
	{
		StartChargeGameplayCue();
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
		&UGA_BossBurstShoot::UpdateTrackingAim,
		FMath::Max(AttackData->AimUpdateInterval, 0.01f),
		true);
	World->GetTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&UGA_BossBurstShoot::FinishTracking,
		AttackData->TrackingDuration,
		false);
}

void UGA_BossBurstShoot::UpdateTrackingAim()
{
	if (RefreshAimFromTarget())
	{
		UpdateTrackingFocus();
	}
}

void UGA_BossBurstShoot::UpdateTrackingFocus()
{
	if (!bTrackingFocusActive || !SourceEnemy)
	{
		return;
	}

	AAIController* OwningAIController = Cast<AAIController>(SourceEnemy->GetController());
	AActor* TargetActor = CurrentTarget.Get();
	if (OwningAIController && IsValid(TargetActor))
	{
		// 몸 회전은 조준 위치 추적 구간에서만 Gameplay Focus가 소유한다.
		OwningAIController->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	}
}

void UGA_BossBurstShoot::ClearTrackingFocus()
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

void UGA_BossBurstShoot::StartChargeGameplayCue()
{
	if (bChargeGameplayCueActive)
	{
		return;
	}

	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		AbilitySystemComponent->AddGameplayCue(
			LastFPSGameplayTags::GameplayCue_Enemy_Boss_BurstCharge);
		bChargeGameplayCueActive = true;
	}
}

void UGA_BossBurstShoot::StopChargeGameplayCue()
{
	if (!bChargeGameplayCueActive)
	{
		return;
	}

	bChargeGameplayCueActive = false;
	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		// 추적 취소와 정상 발사 경로가 동일한 정리 계약을 사용한다.
		AbilitySystemComponent->RemoveGameplayCue(
			LastFPSGameplayTags::GameplayCue_Enemy_Boss_BurstCharge);
	}
}

bool UGA_BossBurstShoot::RefreshAimFromTarget()
{
	if (bEndingAbility || !AttackData || !AimComponent || !SourceEnemy)
	{
		return false;
	}
	if (!SourceEnemy->IsAlive())
	{
		FinishCurrentAbility(true);
		return false;
	}

	AActor* TargetActor = ResolveCombatTarget();
	if (!TargetActor)
	{
		if (AttackData->bCancelIfTargetLost)
		{
			UE_LOG(LogLastFPSBossBurstShoot, Warning,
				TEXT("보스 버스트 공격 취소: Source=%s, 원인=추적 중 타겟을 잃었습니다."),
				*GetNameSafe(SourceEnemy));
			FinishCurrentAbility(true);
		}
		return false;
	}

	CurrentTarget = TargetActor;
	LockedAimLocation = ResolveAimLocation(*TargetActor);
	bHasAimLocation = true;
	AimComponent->SetAimTarget(this, LockedAimLocation);
	return true;
}

void UGA_BossBurstShoot::FinishTracking()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(TrackingUpdateTimerHandle);
	}

	// 잠금 및 연사 단계에서는 조준 위치만 유지하고 몸 회전 Focus는 해제한다.
	ClearTrackingFocus();

	const bool bAimUpdated = RefreshAimFromTarget();
	if (bEndingAbility || !AttackData || !World)
	{
		return;
	}
	if (!bAimUpdated && !bHasAimLocation)
	{
		FinishCurrentAbility(true);
		return;
	}

	// 이 시점 이후 기본 패턴은 저장된 위치를 유지해 플레이어가 회피할 여지를 제공한다.
	AimComponent->SetAimTarget(this, LockedAimLocation);
	if (AttackData->LockDuration <= 0.f)
	{
		StartBurst();
		return;
	}

	World->GetTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&UGA_BossBurstShoot::StartBurst,
		AttackData->LockDuration,
		false);
}

void UGA_BossBurstShoot::StartBurst()
{
	if (bEndingAbility || !AttackData)
	{
		return;
	}

	StopChargeGameplayCue();

	if (AimComponent)
	{
		AimComponent->BeginFiring(this);
	}

	FiredShotCount = 0;
	FireNextProjectile();
}

void UGA_BossBurstShoot::FireNextProjectile()
{
	if (bEndingAbility || !AttackData || !SourceEnemy || !SourceEnemy->IsAlive()
		|| !AttackData->ProjectileData)
	{
		FinishCurrentAbility(true);
		return;
	}

	if (AttackData->bTrackDuringBurst && !RefreshAimFromTarget())
	{
		if (AttackData->bCancelIfTargetLost)
		{
			return;
		}
	}

	FLastFPSProjectileLaunchRequest LaunchRequest;
	LaunchRequest.SourceActor = SourceEnemy;
	LaunchRequest.ProjectileData = AttackData->ProjectileData;
	LaunchRequest.AimTarget = LockedAimLocation;
	LaunchRequest.FallbackAimDirection = SourceEnemy->GetActorForwardVector();
	LaunchRequest.FallbackMuzzleHeight = AttackData->FallbackMuzzleHeight;
	LaunchRequest.bUseEquippedWeapon = AttackData->bUseEquippedWeapon;

	ALastFPSProjectile* SpawnedProjectile = LastFPSProjectileLaunch::SpawnProjectile(LaunchRequest);
	if (!SpawnedProjectile)
	{
		UE_LOG(LogLastFPSBossBurstShoot, Warning,
			TEXT("보스 버스트 투사체 생성 실패: Source=%s, Shot=%d/%d, AimTarget=%s"),
			*GetNameSafe(SourceEnemy),
			FiredShotCount + 1,
			AttackData->BurstCount,
			*LockedAimLocation.ToCompactString());
	}
	else if (AimComponent)
	{
		AimComponent->NotifyShotFired(this);
	}

	++FiredShotCount;
	if (FiredShotCount >= AttackData->BurstCount)
	{
		FinishBurst();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BurstTimerHandle,
			this,
			&UGA_BossBurstShoot::FireNextProjectile,
			FMath::Max(AttackData->ShotInterval, 0.01f),
			false);
	}
	else
	{
		FinishCurrentAbility(true);
	}
}

void UGA_BossBurstShoot::FinishBurst()
{
	if (bEndingAbility || !AttackData)
	{
		return;
	}

	if (AimComponent)
	{
		AimComponent->EndFiring(this);
	}

	if (!AttackData->bKeepAimDuringRecovery && AimComponent)
	{
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
			&UGA_BossBurstShoot::FinishRecovery,
			AttackData->RecoveryDuration,
			false);
	}
	else
	{
		FinishCurrentAbility(true);
	}
}

void UGA_BossBurstShoot::FinishRecovery()
{
	FinishCurrentAbility(false);
}

void UGA_BossBurstShoot::FinishCurrentAbility(const bool bWasCancelled)
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

void UGA_BossBurstShoot::ClearRuntimeTimers()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(TrackingUpdateTimerHandle);
		TimerManager.ClearTimer(PhaseTimerHandle);
		TimerManager.ClearTimer(BurstTimerHandle);
	}
}

AActor* UGA_BossBurstShoot::ResolveCombatTarget() const
{
	if (!SourceEnemy)
	{
		return nullptr;
	}

	AActor* TargetActor = nullptr;
	const AAIController* OwningAIController = Cast<AAIController>(SourceEnemy->GetController());
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

FVector UGA_BossBurstShoot::ResolveAimLocation(const AActor& TargetActor) const
{
	const float AimHeight = AttackData ? AttackData->TargetAimHeight : 0.f;
	return TargetActor.GetActorLocation() + FVector(0.f, 0.f, AimHeight);
}
