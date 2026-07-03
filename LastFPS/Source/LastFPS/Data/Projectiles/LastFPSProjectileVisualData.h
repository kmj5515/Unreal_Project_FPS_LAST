#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSProjectileVisualData.generated.h"

class UParticleSystem;
class UNiagaraSystem;
class USoundBase;

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSProjectileVisualData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Visual")
    TObjectPtr<UParticleSystem> TrailEffect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Visual")
    TObjectPtr<UNiagaraSystem> TrailNiagaraSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Visual")
    FVector TrailEffectScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Visual")
    TObjectPtr<UParticleSystem> ImpactEffect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Visual")
    TObjectPtr<UNiagaraSystem> ImpactNiagaraSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Visual")
    FVector ImpactEffectScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Audio")
    TObjectPtr<USoundBase> ImpactSound;
};
