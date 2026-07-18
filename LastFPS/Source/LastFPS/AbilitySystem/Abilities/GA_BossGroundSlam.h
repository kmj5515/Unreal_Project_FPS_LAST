#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "GA_BossGroundSlam.generated.h"

class ALastFPSEnemyCharacter;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class ULastFPSBossGroundSlamData;

/** 보스 발밑 지면에 지연 후 확장형 충돌 Mesh를 생성하는 서버 권위 어빌리티다. */
UCLASS()
class LASTFPS_API UGA_BossGroundSlam : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BossGroundSlam();

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
	bool StartImpactEventTask();
	bool StartAttackMontage();
	void ExecuteGroundSlam();
	bool SpawnExpandingMeshAttack();
	FTransform ResolveGroundTransform() const;
	void FinishCurrentAbility(bool bWasCancelled);

	UFUNCTION()
	void OnImpactEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageInterrupted();

	UPROPERTY(EditDefaultsOnly, Category="Boss|Ground Slam")
	TObjectPtr<ULastFPSBossGroundSlamData> AttackData;

	UPROPERTY()
	TObjectPtr<ALastFPSEnemyCharacter> SourceEnemy;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ImpactEventTask;

	bool bEndingAbility = false;
	bool bImpactSpawned = false;
};
