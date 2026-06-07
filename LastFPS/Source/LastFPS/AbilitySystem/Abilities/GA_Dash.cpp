#include "AbilitySystem/Abilities/GA_Dash.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/LastFPSHero.h"
#include "Engine/World.h"
#include "Utility/LastFPSTags.h"

UGA_Dash::UGA_Dash()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    const FLastFPSTags& FPSTags = FLastFPSTags::Get();
    FGameplayTagContainer Tags;
    Tags.AddTag(FPSTags.Ability_Dash);
    Tags.AddTag(FPSTags.Input_Dash);
    SetAssetTags(Tags);
}

void UGA_Dash::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->IsAlive() || Hero->GetCombatState() != EMMCombatState::Idle)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Hero->SetCombatState(EMMCombatState::Dashing);

    bool bWaitingForMontageEnd = false;
    if (DashMontage && Hero->GetMesh())
    {
        if (UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance())
        {
            const float PlayedDuration = AnimInstance->Montage_Play(DashMontage, MontagePlayRate);
            if (PlayedDuration > 0.f)
            {
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &UGA_Dash::OnDashMontageEnded);
                AnimInstance->Montage_SetEndDelegate(EndDelegate, DashMontage);
                bWaitingForMontageEnd = true;
            }
        }
    }

    if (!bWaitingForMontageEnd)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                DashTimerHandle,
                this,
                &UGA_Dash::FinishDash,
                DefaultDashDuration,
                false);
        }
        else
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        }
    }
}

void UGA_Dash::FinishDash()
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Dash::OnDashMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != DashMontage)
    {
        return;
    }

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, bInterrupted);
}

void UGA_Dash::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DashTimerHandle);
    }

    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
    {
        if (Hero->GetCombatState() == EMMCombatState::Dashing)
        {
            Hero->SetCombatState(EMMCombatState::Idle);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
