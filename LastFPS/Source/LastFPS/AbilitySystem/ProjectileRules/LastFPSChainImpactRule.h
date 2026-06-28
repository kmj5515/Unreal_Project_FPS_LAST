#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ProjectileRules/LastFPSProjectileImpactRule.h"
#include "LastFPSChainImpactRule.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class LASTFPS_API ULastFPSChainImpactRule : public ULastFPSProjectileImpactRule
{
	GENERATED_BODY()

public:
	virtual void ExecuteImpact(const FLastFPSProjectileImpactContext& Context) const override;

private:
	void ExecuteChainStep(
		const FLastFPSProjectileImpactContext& Context,
		AActor* ChainSource,
		int32 Depth,
		TSet<TWeakObjectPtr<AActor>>& VisitedActors) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain", meta=(AllowPrivateAccess="true", ClampMin="0.0"))
	float Radius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain", meta=(AllowPrivateAccess="true", ClampMin="0"))
	int32 MaxChainDepth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain", meta=(AllowPrivateAccess="true", ClampMin="1"))
	int32 MaxTargetsPerChain = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain", meta=(AllowPrivateAccess="true"))
	TArray<TSubclassOf<UGameplayEffect>> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain|Condition", meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain|Condition", meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer BlockedTargetTags;
};
