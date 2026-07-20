#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "GA_BossLaser.generated.h"

class AActor;
class ALastFPSEnemyCharacter;
class UAbilitySystemComponent;
class UGameplayEffect;
class ULastFPSBossLaserAttackData;
class ULastFPSCombatAimComponent;
struct FGameplayCueParameters;

/** 목표를 예고 추적한 뒤 고정하거나 추적하는 지속 레이저를 발사하는 서버 권한 Ability다. */
UCLASS()
class LASTFPS_API UGA_BossLaser : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BossLaser();

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
	void UpdateTracking();
	void UpdateTrackingPreview();
	void PushPreviewGameplayCue(const FVector& PreviewDirection);
	void FinishTracking();
	void StartBeam();
	void ApplyBeamPulse();
	void FinishBeam();
	void FinishRecovery();
	void FinishCurrentAbility(bool bWasCancelled);
	void ClearRuntimeTimers();
	void UpdateTrackingFocus();
	void ClearTrackingFocus();
	void StartGameplayCue(
		const FGameplayTag& CueTag,
		bool& bCueActive,
		const FGameplayCueParameters* CueParameters = nullptr);
	void StopGameplayCue(const FGameplayTag& CueTag, bool& bCueActive);

	AActor* ResolveCombatTarget() const;
	AActor* ResolveCombatTargetForSource(const ALastFPSEnemyCharacter& Enemy) const;
	bool IsTargetWithinActivationEnvelope(
		const ALastFPSEnemyCharacter& Enemy,
		const AActor& TargetActor) const;
	FVector ResolveTargetLocation(const AActor& TargetActor) const;
	FVector ResolveMuzzleLocation() const;
	FVector ResolveStableAimDirection(
		const FVector& MuzzleLocation,
		const FVector& TargetLocation) const;
	bool RefreshTargetAim();
	bool LockBeamPathFromCurrentTarget();
	void ClipBeamEndToWorld();
	bool DoesTargetPassConditions(AActor& TargetActor) const;
	bool ApplyEffectsToTarget(AActor& TargetActor) const;
	bool ApplyEffectToTarget(
		AActor& TargetActor,
		UAbilitySystemComponent& TargetASC,
		TSubclassOf<UGameplayEffect> EffectClass) const;

	UPROPERTY(EditDefaultsOnly, Category="Boss|Laser")
	TObjectPtr<ULastFPSBossLaserAttackData> AttackData;

	UPROPERTY()
	TObjectPtr<ALastFPSEnemyCharacter> SourceEnemy;

	UPROPERTY()
	TObjectPtr<ULastFPSCombatAimComponent> AimComponent;

	TWeakObjectPtr<AActor> CurrentTarget;
	FVector LockedAimLocation = FVector::ZeroVector;
	FVector LockedBeamDirection = FVector::ForwardVector;
	FVector CurrentBeamStart = FVector::ZeroVector;
	FVector CurrentBeamEnd = FVector::ZeroVector;
	FTimerHandle TrackingUpdateTimerHandle;
	FTimerHandle PhaseTimerHandle;
	FTimerHandle DamageTimerHandle;
	bool bHasTargetAim = false;
	bool bTrackingFocusActive = false;
	bool bChargeGameplayCueActive = false;
	bool bPreviewGameplayCueActive = false;
	bool bBeamGameplayCueActive = false;
	bool bEndingAbility = false;
};
