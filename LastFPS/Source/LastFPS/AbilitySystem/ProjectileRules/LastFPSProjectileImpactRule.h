#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Data/Projectiles/LastFPSProjectileImpactTypes.h"
#include "LastFPSProjectileImpactRule.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class LASTFPS_API ULastFPSProjectileImpactRule : public UObject
{
	GENERATED_BODY()

public:
	virtual void ExecuteImpact(const FLastFPSProjectileImpactContext& Context) const PURE_VIRTUAL(
		ULastFPSProjectileImpactRule::ExecuteImpact, );

protected:
	static UAbilitySystemComponent* GetAbilitySystemComponent(AActor* Actor);
	static bool DoesTargetPassTags(
		UAbilitySystemComponent* TargetASC,
		const FGameplayTagContainer& RequiredTags,
		const FGameplayTagContainer& BlockedTags);
	static void ApplyGameplayEffectsToTarget(
		const FLastFPSProjectileImpactContext& Context,
		AActor* TargetActor,
		const TArray<TSubclassOf<UGameplayEffect>>& Effects);
};
