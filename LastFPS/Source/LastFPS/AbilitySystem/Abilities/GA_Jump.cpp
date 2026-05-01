#include "AbilitySystem/Abilities/GA_Jump.h"
#include "GameFramework/Character.h"

UGA_Jump::UGA_Jump()
{
    // Jump는 one-shot이므로 NonInstanced로 충분
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::NonInstanced;
    // LocalPredicted: 클라이언트에서 즉시 Jump() 호출 → CMC가 물리 예측 처리
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Jump"));
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

bool UGA_Jump::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    // CMC의 CanJump()로 더블점프 횟수·공중 상태 검사
    const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (!Character || !Character->CanJump())
        return false;

    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UGA_Jump::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (Character)
        Character->Jump();

    // Jump는 즉시 종료 — 가변 점프높이(StopJumping)는 Hero가 직접 처리
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
