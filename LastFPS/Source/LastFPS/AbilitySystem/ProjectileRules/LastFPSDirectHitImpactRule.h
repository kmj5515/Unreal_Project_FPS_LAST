#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ProjectileRules/LastFPSProjectileImpactRule.h"
#include "LastFPSDirectHitImpactRule.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class LASTFPS_API ULastFPSDirectHitImpactRule : public ULastFPSProjectileImpactRule
{
	GENERATED_BODY()

public:
	virtual void ExecuteImpact(const FLastFPSProjectileImpactContext& Context) const override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Direct Hit", meta=(AllowPrivateAccess="true"))
	TArray<TSubclassOf<UGameplayEffect>> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Direct Hit|Damage", meta=(AllowPrivateAccess="true"))
	FLastFPSDamageRange DamageRange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Direct Hit|Condition", meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Direct Hit|Condition", meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer BlockedTargetTags;
};
