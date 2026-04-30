#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "LastFPSProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

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
    TObjectPtr<USphereComponent> CollisionComp;

    UPROPERTY(VisibleAnywhere, Category="Projectile")
    TObjectPtr<UStaticMeshComponent> ProjectileMesh;

    // 피격 시 적용할 GE (에디터에서 BP_GE_Damage 할당)
    UPROPERTY(EditDefaultsOnly, Category="Damage")
    TSubclassOf<UGameplayEffect> DamageEffect;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
               UPrimitiveComponent* OtherComp, FVector NormalImpulse,
               const FHitResult& Hit);
};
