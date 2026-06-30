#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "GA_ViolaIceAura.generated.h"

class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_PlayMontageAndWait;
class UAbilitySystemComponent;
class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;
class UParticleSystem;

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
	void ApplyAuraTargetEffects();
	void ApplyAuraTargetEffect(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass, bool bApplyDamage);
	void FinishAura();
	void ReleaseCastingState();
	bool DoesTargetPassAuraTags(AActor* TargetActor) const;
	UAbilitySystemComponent* GetAbilitySystemComponentFromActor(AActor* Actor) const;
	FVector GetAuraOrigin() const;
	void DrawAuraSphere() const;
	void DrawAuraTargetDebug(AActor* TargetActor) const;
	void GetActorsInAuraSphere(TArray<AActor*>& OutActors) const;

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
	
	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Damage")
	TSubclassOf<UGameplayEffect> AuraDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Damage")
	FLastFPSDamageRange DamageRange;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Damage", meta=(ClampMin="0.01"))
	float DamageInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Target Effects")
	TArray<TSubclassOf<UGameplayEffect>> AuraTargetEffects;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Range", meta=(ClampMin="0.0"))
	float AuraRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Condition")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Aura|Condition")
	FGameplayTagContainer BlockedTargetTags;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AuraEffectEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> AuraMontageTask;

	FTimerHandle AuraTargetEffectTimerHandle;
	FTimerHandle AuraDurationTimerHandle;

	bool bAuraEffectCommitted = false;
};
