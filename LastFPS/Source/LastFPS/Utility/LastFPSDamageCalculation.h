#pragma once

#include "CoreMinimal.h"
#include "LastFPSDamageCalculation.generated.h"

struct FGameplayEffectSpec;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSDamageRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0.0"))
	float MinDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0.0"))
	float MaxDamage = 18.f;

	float Roll() const;
};

struct LASTFPS_API FLastFPSDamageResult
{
	float DamageAmount = 0.f;
	bool bCriticalHit = false;
};

namespace LastFPSDamage
{
	LASTFPS_API float RollDamage(const FLastFPSDamageRange& DamageRange);
	LASTFPS_API FLastFPSDamageResult CalculateDamage(
		const FGameplayEffectSpec& Spec,
		const FLastFPSDamageRange& DamageRange);
	LASTFPS_API bool IsDamageGameplayEffect(TSubclassOf<UGameplayEffect> EffectClass);
	LASTFPS_API void ApplySetByCallerDamage(FGameplayEffectSpec& Spec, const FLastFPSDamageResult& DamageResult);
	LASTFPS_API void RollAndApplySetByCallerDamage(
		FGameplayEffectSpec& Spec,
		TSubclassOf<UGameplayEffect> EffectClass,
		const FLastFPSDamageRange& DamageRange);
}
