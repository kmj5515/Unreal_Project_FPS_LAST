#include "AbilitySystem/Abilities/GA_BasicShoot.h"

#include "Character/Components/WeaponComponent.h"
#include "Character/LastFPSHero.h"
#include "Game/LastFPSPlayerController.h"
#include "UI/HUD/LastFPSHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Utility/LastFPSTags.h"

UGA_BasicShoot::UGA_BasicShoot()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    bDrawDebug = true;

    FGameplayTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Ability_Fire);
    Tags.AddTag(LastFPSGameplayTags::Input_Fire);
    SetAssetTags(Tags);

    ActivationBlockedTags.AddTag(LastFPSGameplayTags::State_Combat_Disabled);
}

void UGA_BasicShoot::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->IsAlive())
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_BasicShoot: cannot fire while dead or missing hero"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (Hero->GetCombatState() != EMMCombatState::Idle)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 스프린트 중이었는지 기억해둔다. 사격과 스프린트 해제가 같은 프레임에 일어나면
    // 애니메이션이 전환될 틈이 없어 총을 내린 자세 그대로 발사된다.
    const bool bWasSprinting = Hero->GetIsSprinting();

    UE_LOG(LogTemp, Warning,
        TEXT("[FIRE] WasSprinting=%d  Delay=%.2f  WantsToSprint=%d  Combat=%d  Speed=%.0f"),
        bWasSprinting ? 1 : 0,
        SprintToFireDelay,
        Hero->GetWantsToSprint() ? 1 : 0,
        static_cast<int32>(Hero->GetCombatState()),
        Hero->GetVelocity().Size2D());

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f,
            bWasSprinting ? FColor::Green : FColor::Red,
            FString::Printf(TEXT("FIRE  WasSprinting=%d  Delay=%.2f  Speed=%.0f"),
                bWasSprinting ? 1 : 0, SprintToFireDelay, Hero->GetVelocity().Size2D()));
    }

    Hero->SetWantsToSprint(false);
    if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
    {
        FGameplayTagContainer SprintTags;
        SprintTags.AddTag(LastFPSGameplayTags::Input_Sprint);
        ASC->CancelAbilities(&SprintTags);
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_BasicShoot: CommitAbility failed"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CachedWeapon = GetWeaponComponent();
    UWeaponComponent* Weapon = CachedWeapon.Get();
    if (!Weapon || !Weapon->CanFire())
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_BasicShoot: missing weapon or cannot fire"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Hero->SetCombatState(EMMCombatState::Attacking);
    Hero->MarkCombatEngaged();

    // 전투 상태는 즉시 올려서 에임 포즈가 블렌드되기 시작하게 하고,
    // 실제 발사는 총을 다 든 뒤에 한다.
    UWorld* FireWorld = GetWorld();
    if (bWasSprinting && SprintToFireDelay > 0.f && FireWorld)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FIRE] delayed by %.2fs (raise weapon)"), SprintToFireDelay);
        FireWorld->GetTimerManager().SetTimer(
            RaiseWeaponTimerHandle,
            this,
            &UGA_BasicShoot::BeginFiring,
            SprintToFireDelay,
            false);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[FIRE] immediate  (WasSprinting=%d Delay=%.2f World=%d)"),
            bWasSprinting ? 1 : 0, SprintToFireDelay, FireWorld ? 1 : 0);
        BeginFiring();
    }
}

void UGA_BasicShoot::BeginFiring()
{
    if (!IsActive())
    {
        return;
    }

    Fire();

    if (!IsActive())
    {
        return;
    }

    UWorld* World = GetWorld();
    UWeaponComponent* Weapon = CachedWeapon.Get();
    if (!World || !Weapon)
    {
        FinishAbility();
        return;
    }

    if (bIsAutoFire)
    {
        World->GetTimerManager().SetTimer(
            FireTimerHandle,
            this,
            &UGA_BasicShoot::Fire,
            Weapon->FireRate,
            true);
    }
    else
    {
        World->GetTimerManager().SetTimer(
            FinishAbilityTimerHandle,
            this,
            &UGA_BasicShoot::FinishAbility,
            MinAttackStateDuration,
            false);
    }
}

void UGA_BasicShoot::Fire()
{
    if (!IsActive() || !GetWorld())
    {
        return;
    }

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->IsAlive())
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
        return;
    }

    Hero->MarkCombatEngaged();

    UWeaponComponent* Weapon = CachedWeapon.Get();
    if (!Weapon || !Weapon->CanFire())
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
        return;
    }

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character || !Character->IsLocallyControlled())
    {
        return;
    }

    AController* Controller = Character->GetController();
    if (!Controller)
    {
        return;
    }

    if (!Weapon->TryConsumePredictedRound())
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
        return;
    }

    LocalFire(Weapon);

    FVector CameraLocation;
    FRotator AimRotation;
    Controller->GetPlayerViewPoint(CameraLocation, AimRotation);

    Weapon->FireFromClientAim(
        Weapon->GetMuzzleTransform().GetLocation(),
        CameraLocation,
        AimRotation.Vector(),
        DamageEffectClass,
        ShouldDrawDebug(),
        GetDebugDrawTime());

    Weapon->ApplyFireAimRecoil(Hero->GetIsADS());

    if (!Weapon->CanFire())
    {
        // 마지막 발이 차감된 즉시 사격 상태를 끝낸 뒤 리로드를 시작한다.
        // 빈 탄창에서 FIRE를 한 번 더 눌러야 하는 입력 의존 경로는 사용하지 않는다.
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
        TryStartAutoReload(Hero, Weapon);
    }
}

void UGA_BasicShoot::TryStartAutoReload(ALastFPSHero* Hero, UWeaponComponent* Weapon) const
{
    if (!Hero || !Hero->IsLocallyControlled() || !Weapon || !Weapon->CanReload())
    {
        return;
    }

    Hero->GetAbilitySystemComponent()->TryActivateAbilitiesByTag(
        FGameplayTagContainer(LastFPSGameplayTags::Ability_Reload));
}

void UGA_BasicShoot::LocalFire(UWeaponComponent* Weapon)
{
    if (Weapon)
    {
        Weapon->PlayFireEffects();
		Weapon->PlayFireCameraShake();
    }

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->GetMesh())
    {
        return;
    }

    if (ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(Hero->GetController()))
    {
        if (ULastFPSHUDWidget* HUDWidget = PC->GetHUDWidget())
        {
            HUDWidget->PlayCrosshairFireAnimation();
        }
    }

    UAnimMontage* FireMontage = Hero->GetIsADS() ? ADSFireMontage : HipFireMontage;
    if (!FireMontage)
    {
        FireMontage = HipFireMontage;
    }

    if (FireMontage)
    {
        if (UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance())
        {
            AnimInstance->Montage_Play(FireMontage);
        }
    }
}

void UGA_BasicShoot::StopFireMontage() const
{
    const ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->GetMesh())
    {
        return;
    }

    UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    if (HipFireMontage && AnimInstance->Montage_IsPlaying(HipFireMontage))
    {
        AnimInstance->Montage_Stop(CancelMontageBlendOutTime, HipFireMontage);
    }

    if (ADSFireMontage && ADSFireMontage != HipFireMontage && AnimInstance->Montage_IsPlaying(ADSFireMontage))
    {
        AnimInstance->Montage_Stop(CancelMontageBlendOutTime, ADSFireMontage);
    }
}

void UGA_BasicShoot::FinishAbility()
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_BasicShoot::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(FinishAbilityTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(RaiseWeaponTimerHandle);
    }

    if (bWasCancelled)
    {
        StopFireMontage();
    }

    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
    {
        if (Hero->GetCombatState() == EMMCombatState::Attacking)
        {
            Hero->SetCombatState(EMMCombatState::Idle);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UWeaponComponent* UGA_BasicShoot::GetWeaponComponent() const
{
    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
    {
        return Hero->GetWeaponComponent();
    }

    return nullptr;
}
