#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Sprint.generated.h"

UCLASS()
class LASTFPS_API UGA_Sprint : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Sprint();

    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayTagContainer* SourceTags = nullptr,
                                     const FGameplayTagContainer* TargetTags = nullptr,
                                     FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                  const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                             const FGameplayAbilityActorInfo* ActorInfo,
                             const FGameplayAbilityActivationInfo ActivationInfo,
                             bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    // MoveSpeed Add +300 (에디터에서 BP_GE_SprintSpeed 할당)
    UPROPERTY(EditDefaultsOnly, Category="Sprint")
    TSubclassOf<UGameplayEffect> SprintSpeedEffect;

    // Stamina 주기적 감소 (에디터에서 BP_GE_SprintStaminaDrain 할당)
    UPROPERTY(EditDefaultsOnly, Category="Sprint")
    TSubclassOf<UGameplayEffect> StaminaDrainEffect;

private:
    FActiveGameplayEffectHandle SpeedEffectHandle;
    FActiveGameplayEffectHandle DrainEffectHandle;
    FDelegateHandle StaminaDelegateHandle;

    void OnStaminaChanged(const FOnAttributeChangeData& Data);
};
