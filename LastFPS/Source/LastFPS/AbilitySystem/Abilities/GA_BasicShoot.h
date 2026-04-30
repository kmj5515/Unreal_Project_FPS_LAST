#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_BasicShoot.generated.h"

class UWeaponComponent;

UCLASS()
class LASTFPS_API UGA_BasicShoot : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_BasicShoot();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                  const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                             const FGameplayAbilityActorInfo* ActorInfo,
                             const FGameplayAbilityActivationInfo ActivationInfo,
                             bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    // true = 연사, false = 단발
    UPROPERTY(EditDefaultsOnly, Category="Shoot")
    bool bIsAutoFire = false;

private:
    void Fire();
    UWeaponComponent* GetWeaponComponent() const;

    FTimerHandle FireTimerHandle;

    // ActivateAbility 파라미터를 EndAbility 호출에 재사용
    FGameplayAbilitySpecHandle     CachedHandle;
    const FGameplayAbilityActorInfo* CachedActorInfo  = nullptr;
    FGameplayAbilityActivationInfo CachedActivationInfo;
};
