#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Dash.generated.h"

class ALastFPSHero;
class UAnimMontage;

UCLASS()
class LASTFPS_API UGA_Dash : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Dash();

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
    UPROPERTY(EditDefaultsOnly, Category="Dash|Animation")
    TObjectPtr<UAnimMontage> DashMontage;

    UPROPERTY(EditDefaultsOnly, Category="Dash|Animation", meta=(ClampMin="0.01"))
    float MontagePlayRate = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="Dash|State", meta=(ClampMin="0.0"))
    float DefaultDashDuration = 0.18f;

private:
    void FinishDash();
    void OnDashMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    FTimerHandle DashTimerHandle;
};
