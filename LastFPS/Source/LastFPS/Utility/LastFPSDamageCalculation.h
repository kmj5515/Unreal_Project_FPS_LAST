#pragma once

#include "CoreMinimal.h"
#include "LastFPSDamageCalculation.generated.h"

struct FGameplayEffectSpec;
class UGameplayEffect;

UENUM(BlueprintType)
enum class ELastFPSDamageElement : uint8
{
	Physical UMETA(DisplayName="Physical"),
	Fire UMETA(DisplayName="Fire"),
	Ice UMETA(DisplayName="Ice"),
	Electric UMETA(DisplayName="Electric"),
	Poison UMETA(DisplayName="Poison"),
};

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSDamageRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0.0"))
	float MinDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0.0"))
	float MaxDamage = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage")
	ELastFPSDamageElement DamageElement = ELastFPSDamageElement::Physical;

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
		const FLastFPSDamageRange& DamageRange);
}
