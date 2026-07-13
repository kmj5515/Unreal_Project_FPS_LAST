#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "LastFPSProjectile.generated.h"

class UBoxComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UParticleSystem;
class UParticleSystemComponent;
class UProjectileMovementComponent;
class UPrimitiveComponent;
class ULastFPSProjectileImpactRule;
class ULastFPSProjectileVisualData;

// VFX 전용 투사체 — 데미지는 GA_BasicShoot의 LineTrace가 처리
UCLASS()
class LASTFPS_API ALastFPSProjectile : public AActor
{
    GENERATED_BODY()

public:
    ALastFPSProjectile();

    void InitializeGameplayProjectile(
        AActor* InSourceActor,
        const TArray<TObjectPtr<ULastFPSProjectileImpactRule>>& InImpactRules,
        const TArray<TSubclassOf<UGameplayEffect>>& InLegacyEffectsOnHit,
        ULastFPSProjectileVisualData* InVisualData,
        float InBaseDamageOverride = 0.f);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category="Projectile")
    TObjectPtr<UBoxComponent> CollisionComp;

    UPROPERTY(VisibleAnywhere, Category="Projectile")
    TObjectPtr<UParticleSystemComponent> TrailParticle;

    UPROPERTY(VisibleAnywhere, Category="Projectile")
    TObjectPtr<UNiagaraComponent> TrailNiagara;

    UPROPERTY(EditDefaultsOnly, Category="Projectile")
    TObjectPtr<UParticleSystem> TrailEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Damage")
    FLastFPSDamageRange LegacyDamageRange;

private:
    UFUNCTION()
    void OnProjectileOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnProjectileStop(const FHitResult& ImpactResult);

    void EnableGameplayCollision();
    void ExecuteImpactRules(AActor* HitActor, const FHitResult& ImpactResult);
    void ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass);
    void ApplyVisualData();
    void PlayImpactFeedback(const FHitResult& ImpactResult);

    UPROPERTY()
    TObjectPtr<AActor> SourceActor;

    UPROPERTY()
    TArray<TObjectPtr<ULastFPSProjectileImpactRule>> ImpactRules;

    UPROPERTY()
    TArray<TSubclassOf<UGameplayEffect>> LegacyEffectsOnHit;

    UPROPERTY()
    TObjectPtr<ULastFPSProjectileVisualData> VisualData;

    float BaseDamageOverride = 0.f;

    bool bHasAppliedHit = false;
};
