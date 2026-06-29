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

	// 최초 타격 대상 이후 체인이 몇 단계까지 이어질지 설정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain", meta=(AllowPrivateAccess="true", ClampMin="0", ToolTip="최초 타격 대상 이후 체인이 몇 단계까지 이어질지 설정합니다."))
	int32 MaxChainDepth = 1;

	// 각 체인 대상이 주변 몇 명에게 동시에 퍼질 수 있는지 설정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain", meta=(AllowPrivateAccess="true", ClampMin="1", ToolTip="각 체인 대상이 주변 몇 명에게 동시에 퍼질 수 있는지 설정합니다."))
	int32 MaxTargetsPerChain = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain", meta=(AllowPrivateAccess="true"))
	bool bApplyToInitialTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain", meta=(AllowPrivateAccess="true"))
	TArray<TSubclassOf<UGameplayEffect>> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain|Damage", meta=(AllowPrivateAccess="true"))
	FLastFPSDamageRange DamageRange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain|Condition", meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain|Condition", meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer BlockedTargetTags;
};
