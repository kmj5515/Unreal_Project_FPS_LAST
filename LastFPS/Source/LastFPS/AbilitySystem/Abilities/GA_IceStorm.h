#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSConfirmableAbility.h"
#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"
#include "AbilitySystem/Actors/LastFPSAreaEffectActor.h"
#include "TimerManager.h"
#include "GA_IceStorm.generated.h"

class ALastFPSHero;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UNiagaraComponent;
class UNiagaraSystem;

enum class ELastFPSIceStormPhase : uint8
{
	None,
	Casting,
	Executing
};

UCLASS()
class LASTFPS_API UGA_IceStorm : public ULastFPSActiveGameplayAbility, public ILastFPSConfirmableAbility
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

	virtual bool CanConfirmAbilityInput(FGameplayTag InputTag) const override;
	virtual bool ConfirmAbilityInput(FGameplayTag InputTag) override;
	virtual bool ShouldBlockAbilityInputRelease(FGameplayTag InputTag) const override;

	UFUNCTION(BlueprintCallable, Category="Ice Storm")
	bool ConfirmIceStorm();

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

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Input")
	FGameplayTag ConfirmInputTag;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting", meta=(ClampMin="0.0"))
	float AimTraceRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Area")
	TSubclassOf<ALastFPSAreaEffectActor> AreaEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Area")
	FLastFPSAreaEffectConfig AreaConfig;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting Indicator")
	TObjectPtr<UNiagaraSystem> TargetingIndicatorNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting Indicator", meta=(ClampMin="0.02"))
	float TargetingIndicatorUpdateInterval = 0.03f;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting Indicator")
	FName TargetingIndicatorRadiusNiagaraParameterName = TEXT("User.Radius");

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting Indicator")
	FName TargetingIndicatorVisualRadiusNiagaraParameterName = TEXT("User.VisualRadius");

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting Indicator")
	FName TargetingIndicatorSurfaceNormalNiagaraParameterName = TEXT("User.SurfaceNormal");

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting Indicator")
	FVector TargetingIndicatorLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting Indicator")
	FRotator TargetingIndicatorRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Targeting Indicator")
	FVector TargetingIndicatorScale = FVector::OneVector;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Ground")
	bool bProjectTargetToGround = true;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Ground", meta=(ClampMin="0.0"))
	float GroundTraceStartOffset = 300.f;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Ground", meta=(ClampMin="0.0"))
	float GroundTraceDistance = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category="Ice Storm|Ground", meta=(ClampMin="0.0"))
	float GroundSurfaceOffset = 4.f;

private:
	void StartEventTasks();
	bool PlayIceStormMontage();
	bool JumpToMontageSection(FName SectionName) const;
	bool CacheAimTarget();
	FVector GetCameraAimDirection(const ALastFPSHero* Hero) const;
	FVector GetAimTarget(const ALastFPSHero* Hero, const FVector& CameraAimDirection) const;
	FTransform BuildTargetGroundTransform(const ALastFPSHero* Hero, const FVector& TargetLocation) const;
	void StartTargetingIndicator();
	void UpdateTargetingIndicator();
	void StopTargetingIndicator();
	bool ShouldShowTargetingIndicator() const;
	FTransform BuildTargetingIndicatorTransform(const FTransform& GroundTransform) const;
	void ApplyTargetingIndicatorParameters(const FTransform& IndicatorTransform);
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

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TargetingIndicatorNiagaraComponent;

	FTimerHandle TargetingIndicatorTimerHandle;
	FVector CachedTargetLocation = FVector::ZeroVector;
	FTransform CachedTargetTransform = FTransform::Identity;
	ELastFPSIceStormPhase Phase = ELastFPSIceStormPhase::None;
	bool bCommitted = false;
	bool bAreaSpawned = false;
	bool bHasCachedTargetTransform = false;
};
