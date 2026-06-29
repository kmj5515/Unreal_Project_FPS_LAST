#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "GA_ViolaIceTrail.generated.h"

class UGameplayEffect;
class UNiagaraSystem;
class UParticleSystem;

UCLASS()
class LASTFPS_API UGA_ViolaIceTrail : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ViolaIceTrail();

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

private:
	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail")
	TObjectPtr<UNiagaraSystem> TrailNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail")
	TObjectPtr<UParticleSystem> TrailParticleSystem;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail", meta=(ClampMin="0.0"))
	float TrailDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail", meta=(ClampMin="0.0"))
	float MinMoveSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail")
	FVector TrailRelativeLocation = FVector(0.f, 0.f, -88.f);

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail")
	FVector TrailRelativeScale = FVector::OneVector;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail|Damage")
	FLastFPSDamageRange DamageRange;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail|Damage", meta=(ClampMin="0.01"))
	float DamageInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail|Damage", meta=(ClampMin="0.0"))
	float DamageRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail|Condition")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditDefaultsOnly, Category="Viola|Ice Trail|Condition")
	FGameplayTagContainer BlockedTargetTags;
};
