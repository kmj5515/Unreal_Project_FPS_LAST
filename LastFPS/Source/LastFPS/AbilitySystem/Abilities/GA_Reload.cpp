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

    FGameplayTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Ability_Reload);
    Tags.AddTag(LastFPSGameplayTags::Input_Reload);
    SetAssetTags(Tags);

    ActivationBlockedTags.AddTag(LastFPSGameplayTags::State_Combat_Disabled);
}

void UGA_Reload::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    bReloadCompleted = false;
    bReloadUINotified = false;

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->IsAlive() || Hero->GetCombatState() != EMMCombatState::Idle)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UWeaponComponent* Weapon = Hero->GetWeaponComponent();
    if (!Weapon || !Weapon->CanReload())
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

    const float ConfiguredReloadDuration = Weapon->GetReloadDuration();
    const float ReloadDuration = ConfiguredReloadDuration > 0.f
        ? ConfiguredReloadDuration
        : FMath::Max(DefaultReloadDuration, 0.01f);

    // 몽타주와 무관하게 리로드 시작을 UI에 알린다. 소요 시간은 WeaponComponent가 소유한 값을 사용하므로 여기서 넘기지 않는다.
    Weapon->NotifyReloadStarted();
    bReloadUINotified = true;

    if (ReloadMontage && Hero->GetMesh())
    {
        if (UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance())
        {
            const float DurationMatchedPlayRate = ReloadMontage->GetPlayLength() / ReloadDuration;
            AnimInstance->Montage_Play(
                ReloadMontage,
                FMath::Max(DurationMatchedPlayRate * MontagePlayRate, 0.01f));
        }
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ReloadTimerHandle,
            this,
            &UGA_Reload::FinishReload,
            ReloadDuration,
            false);
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void UGA_Reload::FinishReload()
{
    if (bReloadCompleted)
    {
        return;
    }

    bReloadCompleted = true;
    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
    {
        if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
        {
            Weapon->CompleteReload();
        }
    }

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
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
        if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
        {
            Weapon->RestoreMagazineToWeapon();

            // 시작 알림을 보낸 경우에만 종료 알림을 보내 UI 표시를 정확히 한 쌍으로 닫는다.
            // 정상 완료면 bReloadCompleted, 중간 취소면 false로 전달해 HUD가 상황에 맞게 표시를 정리한다.
            if (bReloadUINotified)
            {
                Weapon->NotifyReloadFinished(bReloadCompleted);
                bReloadUINotified = false;
            }
        }

        if (Hero->GetCombatState() == EMMCombatState::Reloading)
        {
            Hero->SetCombatState(EMMCombatState::Idle);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
