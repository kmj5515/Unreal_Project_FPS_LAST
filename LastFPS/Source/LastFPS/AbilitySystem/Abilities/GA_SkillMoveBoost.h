#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "GA_SkillMoveBoost.generated.h"

class UGameplayEffect;

UCLASS()
class LASTFPS_API UGA_SkillMoveBoost : public ULastFPSGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_SkillMoveBoost();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Skill")
    TSubclassOf<UGameplayEffect> SpeedBoostEffect;
};
