#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSProjectile.generated.h"

class UBoxComponent;
class UParticleSystem;
class UParticleSystemComponent;
class UProjectileMovementComponent;

// VFX 전용 투사체 — 데미지는 GA_BasicShoot의 LineTrace가 처리
UCLASS()
class LASTFPS_API ALastFPSProjectile : public AActor
{
    GENERATED_BODY()

public:
    ALastFPSProjectile();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category="Projectile")
    TObjectPtr<UBoxComponent> CollisionComp;

    UPROPERTY(VisibleAnywhere, Category="Projectile")
    TObjectPtr<UParticleSystemComponent> TrailParticle;

    UPROPERTY(EditDefaultsOnly, Category="Projectile")
    TObjectPtr<UParticleSystem> TrailEffect;
};
