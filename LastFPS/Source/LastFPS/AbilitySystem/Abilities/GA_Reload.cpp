#include "AbilitySystem/Abilities/GA_Reload.h"

#include "AbilitySystemComponent.h"
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
    ReloadWeaponSlot = Weapon->GetActiveWeaponSlot();

    const float ConfiguredReloadDuration = Weapon->GetReloadDuration();
    const float ReloadDuration = ConfiguredReloadDuration > 0.f
        ? ConfiguredReloadDuration
        : FMath::Max(DefaultReloadDuration, 0.01f);

    // 몽타주와 무관하게 리로드 시작을 UI에 알린다. 소요 시간은 WeaponComponent가 소유한 값을 사용하므로 여기서 넘기지 않는다.
    Weapon->NotifyReloadStarted();
    bReloadUINotified = true;

    // AnimInstance 직접 재생은 ASC 의 RepAnimMontageInfo 를 타지 않아 다른 플레이어 화면에서
    // 장전 동작이 보이지 않는다. 완료 판정은 아래 서버 타이머가 그대로 담당한다.
    if (ReloadMontage)
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            const float DurationMatchedPlayRate = ReloadMontage->GetPlayLength() / ReloadDuration;
            ASC->PlayMontage(
                this,
                ActivationInfo,
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
            // 어빌리티가 이미 끝난 뒤(원격 완료 통보 경로)에도 이 타이머는 살아 있다.
            // 그동안 무기를 바꿨다면 다른 무기의 탄창을 채우게 되므로 시작 슬롯과 같을 때만 채운다.
            if (Weapon->GetActiveWeaponSlot() == ReloadWeaponSlot)
            {
                Weapon->CompleteReload();
            }
        }
    }

    if (IsActive())
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
    }
}

void UGA_Reload::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 원격 클라이언트의 완료 통보(ServerEndAbility)는 서버 타이머보다 항상 먼저 도착한다.
    // 서버 타이머는 활성화 RPC가 도착한 뒤에 시작하므로 왕복 지연만큼 늦게 만료된다.
    // 그 통보로 타이머를 지우면 서버 탄창이 갱신되지 않아, 다음 발사에서 서버 값(0)으로 되돌아간다.
    // 취소가 아니라면 서버 타이머를 남겨 두고 장전 완료는 서버 시간으로 판정한다(클라이언트가 장전을 앞당길 수 없다).
    const AActor* Avatar = GetAvatarActorFromActorInfo();
    const bool bKeepAuthorityReloadTimer =
        !bWasCancelled && !bReloadCompleted && Avatar && Avatar->HasAuthority();

    if (UWorld* World = GetWorld())
    {
        if (!bKeepAuthorityReloadTimer)
        {
            World->GetTimerManager().ClearTimer(ReloadTimerHandle);
        }
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
