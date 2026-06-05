#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Reload.generated.h"

class UAnimMontage;

UCLASS()
class LASTFPS_API UGA_Reload : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Reload();

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
    UPROPERTY(EditDefaultsOnly, Category="Reload|Animation")
    TObjectPtr<UAnimMontage> ReloadMontage;

    UPROPERTY(EditDefaultsOnly, Category="Reload|Animation", meta=(ClampMin="0.01"))
    float MontagePlayRate = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="Reload|State", meta=(ClampMin="0.0"))
    float DefaultReloadDuration = 1.f;

private:
    void FinishReload();
    void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    FTimerHandle ReloadTimerHandle;
};
