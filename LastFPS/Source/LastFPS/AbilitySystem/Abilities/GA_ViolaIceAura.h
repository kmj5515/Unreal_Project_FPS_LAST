#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "GA_ViolaIceAura.generated.h"

class ALastFPSAreaEffectActor;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;
struct FLastFPSAreaEffectConfig;

UCLASS()
class LASTFPS_API UGA_ViolaIceAura : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ViolaIceAura();

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
	void ApplyAuraEffect();
	bool CommitAndApplyAuraEffect();

	void StartAuraLoop();
	void SpawnAuraAreaEffect();
	bool ShouldSpawnAuraAreaEffect() const;
	FLastFPSAreaEffectConfig BuildAuraAreaConfig() const;
	FTransform BuildAuraAreaSpawnTransform(const AActor* SourceActor) const;
	void StartAuraTrail();
	void StopAuraTrail();
	void UpdateAuraTrailState();
	float GetAuraTrailSpeed() const;

	void FinishAura();
	void ReleaseCastingState();

	FVector GetCurrentAvatarLocation() const;
	FVector GetAuraSourceLocation() const;
	FVector GetAuraOrigin() const;
	float GetAuraVisualRadius() const;

	UFUNCTION()
	void OnAuraEffectEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnAuraMontageCompleted();

	UFUNCTION()
	void OnAuraMontageCancelled();

	UFUNCTION()
	void OnAuraMontageInterrupted();

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Animation")
	TObjectPtr<UAnimMontage> AuraMontage;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Animation", meta=(ClampMin="0.01"))
	float MontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Event")
	FGameplayTag AuraEffectEventTag;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Effect")
	TSubclassOf<UGameplayEffect> AuraEffect;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Effect", meta=(ClampMin="0.0"))
	float AuraDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Area")
	TSubclassOf<ALastFPSAreaEffectActor> AuraAreaEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Range", meta=(ClampMin="0.0"))
	float AuraRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Area", meta=(ClampMin="0.0"))
	float AuraAreaDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Damage", meta=(ClampMin="0.01"))
	float DamageInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Damage")
	TSubclassOf<UGameplayEffect> AuraDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Damage")
	TSubclassOf<UGameplayEffect> AuraDamageCooldownEffect;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Damage")
	FLastFPSDamageRange DamageRange;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Target Effects")
	TArray<TSubclassOf<UGameplayEffect>> AuraTargetEffects;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|VFX")
	TObjectPtr<UNiagaraSystem> AuraPulseNiagaraSystem;
	
	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|VFX", meta=(ClampMin="0.0"))
	float AuraVisualRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|VFX")
	FName AuraVisualRadiusNiagaraParameterName = TEXT("User.VisualRadius");

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|VFX")
	FName AuraDurationNiagaraParameterName = TEXT("User.Duration");

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Trail")
	TObjectPtr<UNiagaraSystem> AuraTrailNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Trail")
	FName AuraTrailAttachBoneName = TEXT("root");

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Trail")
	FVector AuraTrailAttachLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Trail")
	FRotator AuraTrailAttachRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Trail")
	bool bDeactivateAuraTrailWhenStationary = true;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Trail", meta=(ClampMin="0.0"))
	float AuraTrailMinActivationSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Trail", meta=(ClampMin="0.02"))
	float AuraTrailStateUpdateInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Origin")
	FName AuraOriginBoneName = TEXT("root");

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Ground")
	bool bAlignAuraAreaToGround = true;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Ground")
	TEnumAsByte<ECollisionChannel> AuraGroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Ground", meta=(ClampMin="0.0"))
	float AuraGroundTraceStartOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Ground", meta=(ClampMin="0.0"))
	float AuraGroundTraceDistance = 500.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Ground", meta=(ClampMin="0.0"))
	float AuraGroundSurfaceOffset = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Ground", meta=(ClampMin="0.0"))
	float AuraGroundMinimumSurfaceOffset = 6.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Condition")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Condition")
	FGameplayTagContainer BlockedTargetTags;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AuraEffectEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> AuraMontageTask;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> AuraTrailNiagaraComponent;

	FTimerHandle AuraTargetEffectTimerHandle;
	FTimerHandle AuraDurationTimerHandle;
	FTimerHandle AuraTrailStateTimerHandle;

	bool bAuraEffectCommitted = false;
};
