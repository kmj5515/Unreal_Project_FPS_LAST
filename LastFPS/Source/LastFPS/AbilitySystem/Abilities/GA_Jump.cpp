#include "AbilitySystem/Abilities/GA_Jump.h"
#include "Utility/LastFPSTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Jump::UGA_Jump()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    FGameplayTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Ability_Jump);
    Tags.AddTag(LastFPSGameplayTags::Ability_DoubleJump);
    Tags.AddTag(LastFPSGameplayTags::Input_Jump);
    SetAssetTags(Tags);
}

void UGA_Jump::OnAvatarSet(
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);

    ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    if (Character)
    {
        ApplyJumpSettings(*Character);
    }
}

bool UGA_Jump::CanActivateAbility(
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

    const ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    return Character && CanJumpWithConfiguredMaxCount(*Character);
}

void UGA_Jump::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    if (!Character || !CanJumpWithConfiguredMaxCount(*Character))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ApplyJumpSettings(*Character);
    Character->Jump();
}

void UGA_Jump::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
    {
        Character->StopJumping();
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

int32 UGA_Jump::GetEffectiveMaxJumpCount(const ACharacter& Character) const
{
    return FMath::Max(Character.JumpMaxCount, FMath::Max(1, MaxJumpCount));
}

bool UGA_Jump::CanJumpWithConfiguredMaxCount(const ACharacter& Character) const
{
    if (Character.CanJump())
    {
        return true;
    }

    const UCharacterMovementComponent* MovementComponent = Character.GetCharacterMovement();
    if (!MovementComponent || Character.bIsCrouched || !MovementComponent->CanAttemptJump())
    {
        return false;
    }

    const int32 EffectiveMaxJumpCount = GetEffectiveMaxJumpCount(Character);
    if (Character.JumpCurrentCount == 0 && MovementComponent->IsFalling())
    {
        return Character.JumpCurrentCount + 1 < EffectiveMaxJumpCount;
    }

    return Character.JumpCurrentCount < EffectiveMaxJumpCount;
}

void UGA_Jump::ApplyJumpSettings(ACharacter& Character) const
{
    Character.JumpMaxCount = GetEffectiveMaxJumpCount(Character);
}
