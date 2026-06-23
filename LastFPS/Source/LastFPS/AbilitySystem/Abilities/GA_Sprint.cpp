#include "AbilitySystem/Abilities/GA_Sprint.h"
#include "Utility/LastFPSTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Character/LastFPSHero.h"

UGA_Sprint::UGA_Sprint()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    const FLastFPSTags& FPSTags = FLastFPSTags::Get();
    FGameplayTagContainer Tags;
    Tags.AddTag(FPSTags.Ability_Sprint);
    Tags.AddTag(FPSTags.Input_Sprint);
    SetAssetTags(Tags);
}

bool UGA_Sprint::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    const ALastFPSHero* Hero = ActorInfo ? Cast<ALastFPSHero>(ActorInfo->AvatarActor.Get()) : nullptr;
    return !Hero || Hero->CanStartSprint();
}

void UGA_Sprint::ActivateAbility(
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
    if (!ASC)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(ActorInfo->AvatarActor.Get()))
    {
        if (!Hero->CanStartSprint())
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        Hero->SetSprinting(true);
    }

    // MoveSpeed 어트리뷰트 증가 → CharacterBase 콜백이 CMC에 반영
    if (SprintSpeedEffect)
    {
        FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(SprintSpeedEffect);
        SpeedEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
    }

    // Stamina 주기적 소모
    if (StaminaDrainEffect)
    {
        FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(StaminaDrainEffect);
        DrainEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
    }

    // 혹시 이전 핸들이 남아있으면 먼저 제거 (중복 바인딩 방지)
    auto& StaminaDelegate = ASC->GetGameplayAttributeValueChangeDelegate(
        ULastFPSAttributeSet::GetStaminaAttribute());
    if (StaminaDelegateHandle.IsValid())
        StaminaDelegate.Remove(StaminaDelegateHandle);

    // Stamina 0 → 어빌리티 자동 종료
    StaminaDelegateHandle = StaminaDelegate.AddUObject(this, &UGA_Sprint::OnStaminaChanged);
}

void UGA_Sprint::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
    if (Data.NewValue <= 0.f)
    {
        EndAbility(GetCurrentAbilitySpecHandle(),
                   GetCurrentActorInfo(),
                   GetCurrentActivationInfo(),
                   true, true);
    }
}

void UGA_Sprint::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    if (ActorInfo)
    {
        if (ALastFPSHero* Hero = Cast<ALastFPSHero>(ActorInfo->AvatarActor.Get()))
        {
            Hero->SetSprinting(false);
        }
    }

    if (ASC)
    {
        // GE 제거 → MoveSpeed가 원래값으로 돌아오면 콜백이 CMC 속도도 복원
        ASC->RemoveActiveGameplayEffect(SpeedEffectHandle);
        ASC->RemoveActiveGameplayEffect(DrainEffectHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(
            ULastFPSAttributeSet::GetStaminaAttribute()).Remove(StaminaDelegateHandle);
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
