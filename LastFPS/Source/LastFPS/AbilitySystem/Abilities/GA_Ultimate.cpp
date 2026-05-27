#include "AbilitySystem/Abilities/GA_Ultimate.h"
#include "Utility/LastFPSTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Game/LastFPSPlayerState.h"

UGA_Ultimate::UGA_Ultimate()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

    const FLastFPSTags& FPSTags = FLastFPSTags::Get();
    FGameplayTagContainer Tags;
    Tags.AddTag(FPSTags.Ability_Ultimate);
    Tags.AddTag(FPSTags.Input_Ultimate);
    SetAssetTags(Tags);
}

bool UGA_Ultimate::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
        return false;

    const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!ASC)
        return false;

    const ULastFPSAttributeSet* AS = ASC->GetSet<ULastFPSAttributeSet>();
    if (!AS)
        return false;

    return AS->GetUltimateGauge() >= static_cast<float>(ALastFPSPlayerState::UltimateKillsRequired) - KINDA_SMALL_NUMBER;
}

void UGA_Ultimate::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!ASC)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ALastFPSPlayerState* PS = Cast<ALastFPSPlayerState>(ASC->GetOwnerActor());
    if (!PS)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetUltimateGaugeAttribute(), 0.f);

    PS->Auth_StartUltimateKillHealWindow(ALastFPSPlayerState::UltimateKillHealWindowSeconds);

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
