#include "AbilitySystem/Abilities/GA_SkillHeal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "AbilitySystem/Effects/GE_HealInstant.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Skill2, "Ability.Skill2")

UGA_SkillHeal::UGA_SkillHeal()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

    HealEffect = ULastFPSGE_HealInstant::StaticClass();

    AbilityTags.AddTag(TAG_Ability_Skill2);
}

bool UGA_SkillHeal::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
        return false;

    const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!ASC) return false;

    const ULastFPSAttributeSet* AS = ASC->GetSet<ULastFPSAttributeSet>();
    if (!AS) return false;

    if (AS->GetHealth() >= AS->GetMaxHealth() - KINDA_SMALL_NUMBER)
        return false;

    return true;
}

void UGA_SkillHeal::ActivateAbility(
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

    if (!HealEffect)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(HealEffect);
    ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
