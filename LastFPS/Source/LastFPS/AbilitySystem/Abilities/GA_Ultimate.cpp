#include "AbilitySystem/Abilities/GA_Ultimate.h"
#include "Utility/LastFPSTags.h"
#include "AbilitySystem/Effects/GE_UltimateCooldown.h"

UGA_Ultimate::UGA_Ultimate()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

    // 다른 스킬(Q/E)과 동일하게 쿨다운 기반. CommitAbility 가 이 GE 를 적용한다.
    CooldownGameplayEffectClass = ULastFPSGE_UltimateCooldown::StaticClass();

    FGameplayTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Ability_Ultimate);
    Tags.AddTag(LastFPSGameplayTags::Input_Ultimate);
    SetAssetTags(Tags);

    ActivationBlockedTags.AddTag(LastFPSGameplayTags::State_Combat_Disabled);
}

bool UGA_Ultimate::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    // TODO: 궁극기 쿨다운 기반 재설계. 킬 기반 게이지 게이트는 폐기됨 — 쿨다운(CooldownGameplayEffectClass)
    //       외 추가 발동 조건이 필요해지면 여기서 처리. 현재는 기본 조건만.
    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UGA_Ultimate::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 다른 스킬(Q/E)과 동일하게 쿨다운/코스트를 커밋 (GE_UltimateCooldown 적용, 기본 60초).
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // TODO: 궁극기 효과 재구현(쿨다운 기반). 폐기된 메커닉 = 킬 게이지 충전 / 8초 킬힐(+100HP).
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
