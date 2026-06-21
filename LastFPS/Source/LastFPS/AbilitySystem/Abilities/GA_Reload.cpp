#include "AbilitySystem/Abilities/GA_Reload.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Components/WeaponComponent.h"
#include "Character/LastFPSHero.h"
#include "Engine/World.h"
#include "Utility/LastFPSTags.h"

UGA_Reload::UGA_Reload()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    const FLastFPSTags& FPSTags = FLastFPSTags::Get();
    FGameplayTagContainer Tags;
    Tags.AddTag(FPSTags.Ability_Reload);
    Tags.AddTag(FPSTags.Input_Reload);
    SetAssetTags(Tags);
}

void UGA_Reload::ActivateAbility(
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

    Hero->SetCombatState(EMMCombatState::Reloading);

    if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
    {
        Weapon->PlayReloadAnimation();
    }

    bool bWaitingForMontageEnd = false;
    if (ReloadMontage && Hero->GetMesh())
    {
        if (UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance())
        {
            const float PlayedDuration = AnimInstance->Montage_Play(ReloadMontage, MontagePlayRate);
            if (PlayedDuration > 0.f)
            {
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &UGA_Reload::OnReloadMontageEnded);
                AnimInstance->Montage_SetEndDelegate(EndDelegate, ReloadMontage);
                bWaitingForMontageEnd = true;
            }
        }
    }

    if (!bWaitingForMontageEnd)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                ReloadTimerHandle,
                this,
                &UGA_Reload::FinishReload,
                DefaultReloadDuration,
                false);
        }
        else
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        }
    }
}

void UGA_Reload::FinishReload()
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Reload::OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != ReloadMontage)
    {
        return;
    }

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, bInterrupted);
}

void UGA_Reload::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReloadTimerHandle);
    }

    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
    {
        if (Hero->GetCombatState() == EMMCombatState::Reloading)
        {
            Hero->SetCombatState(EMMCombatState::Idle);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
