#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"
#include "GA_SkillHeal.generated.h"

class UGameplayEffect;

UCLASS()
class LASTFPS_API UGA_SkillHeal : public ULastFPSActiveGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_SkillHeal();

    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                    const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayTagContainer* SourceTags = nullptr,
                                    const FGameplayTagContainer* TargetTags = nullptr,
                                    FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Skill")
    TSubclassOf<UGameplayEffect> HealEffect;
};
