#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"
#include "GA_Projectile.generated.h"

class ALastFPSHero;
class UAbilityTask_WaitGameplayEvent;
class ULastFPSAbilityProjectileData;
struct FGameplayAbilityTargetDataHandle;

UCLASS()
class LASTFPS_API UGA_Projectile : public ULastFPSActiveGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Projectile();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                  const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                             const FGameplayAbilityActorInfo* ActorInfo,
                             const FGameplayAbilityActivationInfo ActivationInfo,
                             bool bReplicateEndAbility,
                             bool bWasCancelled) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Projectile")
    TObjectPtr<ULastFPSAbilityProjectileData> ProjectileData;

    /** 클라이언트 카메라 위치가 서버 시점에서 벗어날 수 있는 허용 거리다. */
    UPROPERTY(EditDefaultsOnly, Category="Projectile|Network",
        meta=(ClampMin="0.0", Units="cm"))
    float MaxClientCameraLocationError = 200.f;

    /** 비정상적인 클라이언트 조준 방향은 서버 시점 방향으로 대체한다. */
    UPROPERTY(EditDefaultsOnly, Category="Projectile|Network",
        meta=(ClampMin="0.0", ClampMax="180.0", Units="deg"))
    float MaxClientAimAngleErrorDegrees = 45.f;

private:
    void SpawnProjectile();
    void CaptureAndSubmitLocalAim();
    void RegisterReplicatedAimCallback();
    void UnregisterReplicatedAimCallback();
    void CacheAimFromView(
        ALastFPSHero& Hero,
        const FVector& ViewLocation,
        const FVector& AimDirection);
    float GetEffectiveAimTraceRange() const;
    void TrySpawnProjectile();
    void HandleReplicatedAimData(
        const FGameplayAbilityTargetDataHandle& Data,
        FGameplayTag ActivationTag);

    UFUNCTION()
    void OnProjectileSpawnEvent(FGameplayEventData Payload);

    UFUNCTION()
    void OnAbilityEndEvent(FGameplayEventData Payload);

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent> ProjectileSpawnEventTask;

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent> AbilityEndEventTask;

    bool bProjectileSpawned = false;
    bool bProjectileSpawnEventReceived = false;
    bool bAimDataSubmitted = false;
    bool bHasCachedAimTarget = false;
    FVector CachedAimTarget = FVector::ZeroVector;
    FDelegateHandle ReplicatedAimDataHandle;
};
