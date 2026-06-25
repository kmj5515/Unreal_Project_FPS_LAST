#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Projectile.generated.h"

class ALastFPSHero;
class UAbilityTask_WaitGameplayEvent;
class ULastFPSAbilityProjectileData;

UCLASS()
class LASTFPS_API UGA_Projectile : public UGameplayAbility
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

private:
    void SpawnProjectile();

    UFUNCTION()
    void OnProjectileSpawnEvent(FGameplayEventData Payload);

    UFUNCTION()
    void OnAbilityEndEvent(FGameplayEventData Payload);

    FVector GetCameraAimDirection(const ALastFPSHero* Hero) const;
    FVector GetAimTarget(const ALastFPSHero* Hero, const FVector& CameraAimDirection) const;

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent> ProjectileSpawnEventTask;

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent> AbilityEndEventTask;

    bool bProjectileSpawned = false;
};
