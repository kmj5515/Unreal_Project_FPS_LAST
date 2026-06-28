#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ProjectileRules/LastFPSProjectileImpactRule.h"
#include "Engine/DataAsset.h"
#include "LastFPSAbilityProjectileData.generated.h"

class ALastFPSProjectile;
class UAnimMontage;
class UGameplayEffect;
class ULastFPSProjectileVisualData;

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSAbilityProjectileData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
    TSubclassOf<ALastFPSProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0.0"))
    float ProjectileSpeed = 3000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Aim", meta=(ClampMin="0.0"))
    float AimTraceRange = 10000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
    FName SpawnSocketName = TEXT("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
    FVector SpawnLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category="Projectile|Impact")
    TArray<TObjectPtr<ULastFPSProjectileImpactRule>> ImpactRules;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Effects", meta=(DeprecatedProperty, DeprecationMessage="Use ImpactRules with ULastFPSDirectHitImpactRule instead."))
    TArray<TSubclassOf<UGameplayEffect>> EffectsOnHit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Visual")
    TObjectPtr<ULastFPSProjectileVisualData> VisualData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Animation")
    TObjectPtr<UAnimMontage> CastMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Animation", meta=(ClampMin="0.01"))
    float MontagePlayRate = 1.f;
};
