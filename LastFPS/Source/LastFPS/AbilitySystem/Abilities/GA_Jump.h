#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"
#include "GA_Jump.generated.h"

class ACharacter;

UCLASS()
class LASTFPS_API UGA_Jump : public ULastFPSActiveGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Jump();

    virtual void OnAvatarSet(
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilitySpec& Spec) override;

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags     = nullptr,
        const FGameplayTagContainer* TargetTags     = nullptr,
        FGameplayTagContainer*       OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Jump", meta=(ClampMin="1", UIMin="1"))
    int32 MaxJumpCount = 2;

private:
    int32 GetEffectiveMaxJumpCount(const ACharacter& Character) const;
    bool CanJumpWithConfiguredMaxCount(const ACharacter& Character) const;
    void ApplyJumpSettings(ACharacter& Character) const;
};
