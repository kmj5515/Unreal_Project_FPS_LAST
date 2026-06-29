#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Data/Projectiles/LastFPSProjectileImpactTypes.h"
#include "Utility/LastFPSDamageCalculation.h"
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
		const TArray<TSubclassOf<UGameplayEffect>>& Effects,
		const FLastFPSDamageRange& DamageRange);

	void DrawDebugPoint(const FLastFPSProjectileImpactContext& Context, const FVector& Location) const;
	void DrawDebugSphere(const FLastFPSProjectileImpactContext& Context, const FVector& Location, float Radius) const;
	void DrawDebugLine(const FLastFPSProjectileImpactContext& Context, const FVector& Start, const FVector& End) const;

private:
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, Category="Debug", meta=(ClampMin="0.0", EditCondition="bDrawDebug"))
	float DebugDrawTime = 2.f;

	UPROPERTY(EditAnywhere, Category="Debug", meta=(EditCondition="bDrawDebug"))
	FLinearColor DebugColor = FLinearColor(0.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category="Debug", meta=(ClampMin="1.0", EditCondition="bDrawDebug"))
	float DebugPointSize = 12.f;

	UPROPERTY(EditAnywhere, Category="Debug", meta=(ClampMin="0.0", EditCondition="bDrawDebug"))
	float DebugLineThickness = 2.f;
};
