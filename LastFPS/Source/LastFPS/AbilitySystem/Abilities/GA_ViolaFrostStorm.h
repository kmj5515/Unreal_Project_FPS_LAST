#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"
#include "AbilitySystem/Actors/LastFPSAreaEffectActor.h"
#include "GA_ViolaFrostStorm.generated.h"

class ALastFPSHero;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;

UCLASS()
class LASTFPS_API UGA_ViolaFrostStorm : public ULastFPSActiveGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ViolaFrostStorm();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	bool PlayFrostStormMontage();
	void StartEffectEventTask();
	void EndEffectEventTask();
	FVector GetAimDirection(const ALastFPSHero* Hero) const;
	FTransform BuildSpawnTransform(const ALastFPSHero* Hero) const;
	FLastFPSAreaEffectConfig BuildAreaConfig() const;
	void SpawnFrostStorm(ALastFPSHero* Hero);
	void SpawnFrostStormFromCurrentAvatar();
	void ReleaseCastingState();

	UFUNCTION()
	void OnFrostStormEffectEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnFrostStormMontageCompleted();

	UFUNCTION()
	void OnFrostStormMontageCancelled();

	UFUNCTION()
	void OnFrostStormMontageInterrupted();

	UPROPERTY(EditDefaultsOnly, Category="Viola|Frost Storm|Animation")
	TObjectPtr<UAnimMontage> FrostStormMontage;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Frost Storm|Animation", meta=(ClampMin="0.01"))
	float MontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Frost Storm|Event")
	FGameplayTag FrostStormEffectEventTag;
	
	UPROPERTY(EditDefaultsOnly, Category="Viola|Frost Storm|Area")
	TSubclassOf<ALastFPSAreaEffectActor> AreaEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Frost Storm|Area")
	FLastFPSAreaEffectConfig AreaConfig;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> FrostStormEffectEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> FrostStormMontageTask;

	bool bFrostStormSpawned = false;
};
