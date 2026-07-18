#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "GA_BossBurstShoot.generated.h"

class AActor;
class ALastFPSEnemyCharacter;
class ULastFPSBossBurstAttackData;
class ULastFPSCombatAimComponent;

/** 타겟을 추적한 뒤 위치를 고정하고 여러 발을 연속 발사하는 보스 전용 Ability다. */
UCLASS()
class LASTFPS_API UGA_BossBurstShoot : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BossBurstShoot();

	virtual bool CanActivateAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	void StartTracking();
	void UpdateTrackingAim();
	void UpdateTrackingFocus();
	void ClearTrackingFocus();
	void StartChargeGameplayCue();
	void StopChargeGameplayCue();
	bool RefreshAimFromTarget();
	void FinishTracking();
	void StartBurst();
	void FireNextProjectile();
	void FinishBurst();
	void FinishRecovery();
	void FinishCurrentAbility(bool bWasCancelled);
	void ClearRuntimeTimers();
	AActor* ResolveCombatTarget() const;
	FVector ResolveAimLocation(const AActor& TargetActor) const;

	UPROPERTY(EditDefaultsOnly, Category="Boss|Burst Shoot")
	TObjectPtr<ULastFPSBossBurstAttackData> AttackData;

	UPROPERTY()
	TObjectPtr<ALastFPSEnemyCharacter> SourceEnemy;

	UPROPERTY()
	TObjectPtr<ULastFPSCombatAimComponent> AimComponent;

	TWeakObjectPtr<AActor> CurrentTarget;
	FVector LockedAimLocation = FVector::ZeroVector;
	int32 FiredShotCount = 0;
	FTimerHandle TrackingUpdateTimerHandle;
	FTimerHandle PhaseTimerHandle;
	FTimerHandle BurstTimerHandle;
	bool bHasAimLocation = false;
	bool bTrackingFocusActive = false;
	bool bChargeGameplayCueActive = false;
	bool bEndingAbility = false;
};
