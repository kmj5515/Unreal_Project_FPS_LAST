#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "AbilitySystem/Actors/LastFPSAreaEffectActor.h"
#include "GA_IceStorm.generated.h"

class ALastFPSHero;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;

enum class ELastFPSIceStormPhase : uint8
{
	None,
	Casting,
	Executing
};

UCLASS()
class LASTFPS_API UGA_IceStorm : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_IceStorm();

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

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category="Ice Storm")
	void ConfirmIceStorm();

	UFUNCTION(BlueprintCallable, Category="Ice Storm")
	void CancelIceStorm();

protected:
	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Animation")
	TObjectPtr<UAnimMontage> IceStormMontage;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Animation", meta=(ClampMin="0.01"))
	float MontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Animation")
	FName CastSectionName = TEXT("CastLoop");

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Animation")
	FName FireSectionName = TEXT("Fire");

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Event")
	FGameplayTag ConfirmEventTag;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Event")
	FGameplayTag SpawnEventTag;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Event")
	FGameplayTag AbilityEndEventTag;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting", meta=(ClampMin="0.0"))
	float AimTraceRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Area")
	TSubclassOf<ALastFPSAreaEffectActor> AreaEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Area")
	FLastFPSAreaEffectConfig AreaConfig;

private:
	void StartEventTasks();
	bool PlayIceStormMontage();
	bool JumpToMontageSection(FName SectionName) const;
	bool CacheAimTarget();
	FVector GetCameraAimDirection(const ALastFPSHero* Hero) const;
	FVector GetAimTarget(const ALastFPSHero* Hero, const FVector& CameraAimDirection) const;
	void SpawnAreaEffect();
	void ReleaseCastingState();
	void EndEventTasks();
	void DrawTargetDebug() const;

	UFUNCTION()
	void OnConfirmEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnSpawnEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnAbilityEndEvent(FGameplayEventData Payload);

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ConfirmEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> SpawnEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AbilityEndEventTask;

	FVector CachedTargetLocation = FVector::ZeroVector;
	ELastFPSIceStormPhase Phase = ELastFPSIceStormPhase::None;
	bool bCommitted = false;
	bool bAreaSpawned = false;
};
