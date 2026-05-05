#include "AbilitySystem/Abilities/GA_SkillMoveBoost.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_MoveSpeedBuff.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Skill1, "Ability.Skill1")

UGA_SkillMoveBoost::UGA_SkillMoveBoost()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

    SpeedBoostEffect = ULastFPSGE_MoveSpeedBuff::StaticClass();

    AbilityTags.AddTag(TAG_Ability_Skill1);
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
