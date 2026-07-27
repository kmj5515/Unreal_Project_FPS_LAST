#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "TimerManager.h"
#include "GA_BossWeakpointKnockdown.generated.h"

class ALastFPSEnemyCharacter;
class UAbilityTask_PlayMontageAndWait;
class ULastFPSBossKnockdownData;

/** 약점 파괴 이벤트를 받아 AI 행동과 공격을 잠그고 넉다운 몽타주를 완주한다. */
UCLASS()
class LASTFPS_API UGA_BossWeakpointKnockdown : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BossWeakpointKnockdown();

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
	bool StartKnockdownMontage();
	float ResolveStartSectionDuration() const;
	void FinishCurrentAbility(bool bWasCancelled);
	void RequestEndSection();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageInterrupted();

	UPROPERTY(EditDefaultsOnly, Category="Boss|Weakpoint Knockdown")
	TObjectPtr<ULastFPSBossKnockdownData> KnockdownData;

	UPROPERTY()
	TObjectPtr<ALastFPSEnemyCharacter> SourceEnemy;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	FTimerHandle EndSectionRequestTimerHandle;
	bool bEndingAbility = false;
};
