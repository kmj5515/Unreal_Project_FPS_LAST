#include "AbilitySystem/Abilities/GA_SkillMoveBoost.h"
#include "Utility/LastFPSTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_MoveSpeedBuff.h"
#include "AbilitySystem/Effects/GE_Skill1Cooldown.h"

UGA_SkillMoveBoost::UGA_SkillMoveBoost()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

    SpeedBoostEffect = ULastFPSGE_MoveSpeedBuff::StaticClass();
    CooldownGameplayEffectClass = ULastFPSGE_Skill1Cooldown::StaticClass();

    const FLastFPSTags& FPSTags = FLastFPSTags::Get();
    FGameplayTagContainer Tags;
    Tags.AddTag(FPSTags.Ability_Skill1);
    Tags.AddTag(FPSTags.Input_Skill1);
    SetAssetTags(Tags);
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
