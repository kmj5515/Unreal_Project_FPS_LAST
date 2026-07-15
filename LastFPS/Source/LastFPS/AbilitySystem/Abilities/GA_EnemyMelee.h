#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "GA_EnemyMelee.generated.h"

class ALastFPSCharacterBase;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class ULastFPSEnemyMeleeAttackData;

/** 적 AI가 Montage 노티파이 시점에 서버 권위 근접 판정을 수행하는 어빌리티다. */
UCLASS()
class LASTFPS_API UGA_EnemyMelee : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnemyMelee();

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
	bool StartAttackMontage();
	void StartHitEventTask();
	void PerformMeleeHit(ALastFPSCharacterBase& SourceCharacter);
	bool ApplyEffectsToTarget(ALastFPSCharacterBase& SourceCharacter, AActor& TargetActor) const;
	void FinishCurrentAbility(bool bWasCancelled);

	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageInterrupted();

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Melee")
	TObjectPtr<ULastFPSEnemyMeleeAttackData> AttackData;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> HitEventTask;

	bool bHitEventConsumed = false;
};
