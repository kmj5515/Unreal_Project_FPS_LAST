#include "AbilitySystem/Abilities/GA_SkillHeal.h"
#include "Utility/LastFPSTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "AbilitySystem/Effects/GE_HealInstant.h"
#include "AbilitySystem/Effects/GE_Cooldown.h"

UGA_SkillHeal::UGA_SkillHeal()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 예측으로 쿨다운 GE 가 즉시 걸려 HUD 스킬 아이콘이 왕복 지연 없이 반응한다.
    // 서버가 거절하면 예측 키에 묶인 회복·쿨다운이 함께 롤백된다.
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    HealEffect = ULastFPSGE_HealInstant::StaticClass();
    CooldownGameplayEffectClass = ULastFPSGE_Cooldown::StaticClass();

    FGameplayTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Ability_Skill2);
    Tags.AddTag(LastFPSGameplayTags::Input_Skill2);
    SetAssetTags(Tags);

    ActivationBlockedTags.AddTag(LastFPSGameplayTags::State_Combat_Disabled);
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

    if (!ActorInfo)
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

    if (HealEffect)
    {
        const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(HealEffect);
        ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
