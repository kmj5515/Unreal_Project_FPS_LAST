#include "AbilitySystem/Abilities/GA_BasicShoot.h"
#include "Character/Components/WeaponComponent.h"
#include "Character/LastFPSHero.h"
#include "Weapons/LastFPSProjectile.h"
#include "NativeGameplayTags.h"
#include "Engine/World.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Fire, "Ability.Fire")

UGA_BasicShoot::UGA_BasicShoot()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    AbilityTags.AddTag(TAG_Ability_Fire);
}

void UGA_BasicShoot::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    const ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->IsAlive())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CachedWeapon = GetWeaponComponent();
    UWeaponComponent* Weapon = CachedWeapon.Get();
    if (!Weapon || !Weapon->CanFire())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Fire();

    if (bIsAutoFire)
    {
        GetWorld()->GetTimerManager().SetTimer(
            FireTimerHandle,
            this, &UGA_BasicShoot::Fire,
            Weapon->FireRate, true);
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UGA_BasicShoot::Fire()
{
    const ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->IsAlive())
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
        return;
    }

    UWeaponComponent* Weapon = CachedWeapon.Get();
    if (!Weapon || !Weapon->CanFire())
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
        return;
    }

    // 발사체 스폰은 서버 전용 (클라이언트는 이후 GameplayCue로 이펙트 처리)
    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Character && Character->HasAuthority() && Weapon->ProjectileClass)
    {
        AController* Controller = Character->GetController();
        if (!Controller) return;

        FVector  CameraLocation;
        FRotator AimRotation;
        Controller->GetPlayerViewPoint(CameraLocation, AimRotation);

        FVector MuzzleLocation = Weapon->GetMuzzleTransform().GetLocation();
        const FVector CameraTraceEnd = CameraLocation + (AimRotation.Vector() * 10000.f);

        FHitResult CameraHitResult;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponTrace), false, Character);
        QueryParams.AddIgnoredActor(Character);

        const bool bHasCameraHit = GetWorld()->LineTraceSingleByChannel(
            CameraHitResult,
            CameraLocation,
            CameraTraceEnd,
            ECC_Visibility,
            QueryParams);

        const FVector AimTarget = bHasCameraHit ? CameraHitResult.ImpactPoint : CameraTraceEnd;
        const FRotator ProjectileRotation = (AimTarget - MuzzleLocation).Rotation();

        FActorSpawnParameters Params;
        Params.Instigator = Character;
        Params.Owner      = Character;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        GetWorld()->SpawnActor<ALastFPSProjectile>(
            Weapon->ProjectileClass,
            MuzzleLocation,
            ProjectileRotation,
            Params);
    }

    Weapon->PlayFireEffects();

    Weapon->AddHeat();

    // 오버히트 도달 시 어빌리티 종료 → 연사 타이머 자동 정지
    if (!Weapon->CanFire())
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
        GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UWeaponComponent* UGA_BasicShoot::GetWeaponComponent() const
{
    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
        return Hero->GetWeaponComponent();
    return nullptr;
}
