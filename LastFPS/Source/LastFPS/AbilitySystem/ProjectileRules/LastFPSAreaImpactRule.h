#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ProjectileRules/LastFPSProjectileImpactRule.h"
#include "LastFPSAreaImpactRule.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class LASTFPS_API ULastFPSAreaImpactRule : public ULastFPSProjectileImpactRule
{
	GENERATED_BODY()

public:
	virtual void ExecuteImpact(const FLastFPSProjectileImpactContext& Context) const override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area", meta=(AllowPrivateAccess="true", ClampMin="0.0"))
	float Radius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area", meta=(AllowPrivateAccess="true", ClampMin="1"))
	int32 MaxTargets = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area", meta=(AllowPrivateAccess="true"))
	bool bIncludeDirectHitTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area", meta=(AllowPrivateAccess="true"))
	TArray<TSubclassOf<UGameplayEffect>> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Damage", meta=(AllowPrivateAccess="true"))
	FLastFPSDamageRange DamageRange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Condition", meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Condition", meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer BlockedTargetTags;
};
