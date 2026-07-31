#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pooling/LastFPSPoolableActor.h"
#include "TimerManager.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "LastFPSProjectile.generated.h"

class UAudioComponent;
class UBoxComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UParticleSystem;
class UParticleSystemComponent;
class UProjectileMovementComponent;
class UPrimitiveComponent;
class UActorComponent;
class ULastFPSProjectileImpactRule;
class ULastFPSProjectileVisualData;
class USoundBase;
struct FLastFPSProjectileAudioSettings;
struct FLastFPSProjectileCollisionSettings;

// VFX 전용 투사체 — 데미지는 GA_BasicShoot의 LineTrace가 처리
UCLASS()
class LASTFPS_API ALastFPSProjectile
    : public AActor
    , public ILastFPSPoolableActor
{
    GENERATED_BODY()

public:
    ALastFPSProjectile();

    void InitializeGameplayProjectile(
        AActor* InSourceActor,
        const TArray<TObjectPtr<ULastFPSProjectileImpactRule>>& InImpactRules,
        const TArray<TSubclassOf<UGameplayEffect>>& InLegacyEffectsOnHit,
        ULastFPSProjectileVisualData* InVisualData,
        float InBaseDamageOverride = 0.f,
        const FLastFPSProjectileCollisionSettings* InCollisionSettings = nullptr,
        const FLastFPSProjectileAudioSettings* InAudioSettings = nullptr);

    /** 풀링 Actor의 Destroy를 대신하는 수명 타이머를 설정한다. */
    void SetGameplayLifeSpan(float LifeSeconds);

    /** 게임플레이를 시작하지 않고 지정한 비주얼 변형의 Trail과 Impact PSO를 요청한다. */
    void PrepareRenderWarmup(ULastFPSProjectileVisualData* InVisualData);

    virtual void OnAcquiredFromPool_Implementation() override;
    virtual void OnReleasedToPool_Implementation() override;
    virtual void OnPrepareForPoolRenderWarmup_Implementation() override;

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

    UPROPERTY(VisibleAnywhere, Category="Projectile|Audio")
    TObjectPtr<UAudioComponent> FlightAudio;

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

    void EnableGameplayCollision(const FLastFPSProjectileCollisionSettings* CollisionSettings);
    void ExecuteImpactRules(AActor* HitActor, const FHitResult& ImpactResult);
    void ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass);
    void ApplyVisualData();
    void ClearRenderWarmupComponents();
    void AddRenderWarmupEffectComponents();
    void ApplyFlightAudio(const FLastFPSProjectileAudioSettings* AudioSettings);
    void StopFlightAudio();
    void SetFlightAudioLocal(
        USoundBase* FlightSound,
        float VolumeMultiplier,
        float PitchMultiplier);
    void PlayImpactFeedback(const FHitResult& ImpactResult);
    void FinishProjectile();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_ApplyFlightAudio(
        USoundBase* FlightSound,
        float VolumeMultiplier,
        float PitchMultiplier);

    UPROPERTY()
    TObjectPtr<AActor> SourceActor;

    UPROPERTY()
    TArray<TObjectPtr<ULastFPSProjectileImpactRule>> ImpactRules;

    UPROPERTY()
    TArray<TSubclassOf<UGameplayEffect>> LegacyEffectsOnHit;

    UPROPERTY()
    TObjectPtr<ULastFPSProjectileVisualData> VisualData;

    /** Impact처럼 평상시 Actor에 없는 효과를 PSO 수집용으로만 잠시 유지한다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UActorComponent>> RenderWarmupComponents;

    float BaseDamageOverride = 0.f;

    bool bHasAppliedHit = false;
    FTimerHandle GameplayLifeTimerHandle;

    UPROPERTY(EditDefaultsOnly, Category="Projectile", meta=(ClampMin="0.0", Units="s"))
    float DefaultGameplayLifeSpan = 1.5f;
};
