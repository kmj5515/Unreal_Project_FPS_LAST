#include "AbilitySystem/Abilities/GA_SkillMoveBoost.h"
#include "Utility/LastFPSTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_MoveSpeedBuff.h"
#include "AbilitySystem/Effects/GE_Cooldown.h"

UGA_SkillMoveBoost::UGA_SkillMoveBoost()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 예측으로 쿨다운 GE 가 즉시 걸려 HUD 스킬 아이콘이 왕복 지연 없이 반응한다.
    // 서버가 거절하면 예측 키에 묶인 속도 버프와 쿨다운이 함께 롤백된다.
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    SpeedBoostEffect = ULastFPSGE_MoveSpeedBuff::StaticClass();
    CooldownGameplayEffectClass = ULastFPSGE_Cooldown::StaticClass();

    FGameplayTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Ability_Skill1);
    Tags.AddTag(LastFPSGameplayTags::Input_Skill1);
    SetAssetTags(Tags);

    ActivationBlockedTags.AddTag(LastFPSGameplayTags::State_Combat_Disabled);
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
