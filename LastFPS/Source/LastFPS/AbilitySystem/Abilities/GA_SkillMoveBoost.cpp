#include "AbilitySystem/Abilities/GA_SkillMoveBoost.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_MoveSpeedBuff.h"

UGA_SkillMoveBoost::UGA_SkillMoveBoost()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

    SpeedBoostEffect = ULastFPSGE_MoveSpeedBuff::StaticClass();

    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Skill1"));
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UGA_SkillMoveBoost::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!ASC || !SpeedBoostEffect)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(SpeedBoostEffect);
    ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
