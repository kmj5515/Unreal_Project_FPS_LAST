#include "AbilitySystem/Abilities/GA_BasicShoot.h"
#include "Character/Components/WeaponComponent.h"
#include "Character/LastFPSHero.h"
#include "Weapons/LastFPSProjectile.h"

UGA_BasicShoot::UGA_BasicShoot()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Fire"));
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UGA_BasicShoot::ActivateAbility(
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

    UWeaponComponent* Weapon = GetWeaponComponent();
    if (!Weapon || !Weapon->CanFire())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 파라미터 캐싱 (타이머 콜백에서 EndAbility 호출 시 필요)
    CachedHandle         = Handle;
    CachedActorInfo      = ActorInfo;
    CachedActivationInfo = ActivationInfo;

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
    UWeaponComponent* Weapon = GetWeaponComponent();
    if (!Weapon || !Weapon->CanFire())
    {
        EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
        return;
    }

    // 발사체 스폰은 서버 전용 (클라이언트는 이후 GameplayCue로 이펙트 처리)
    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Character && Character->HasAuthority() && Weapon->ProjectileClass)
    {
        // 카메라 위치·방향 기준으로 발사 (MuzzleSocket 유무 무관하게 조준선 방향 보장)
        FVector  SpawnLocation;
        FRotator SpawnRotation;
        Character->GetController()->GetPlayerViewPoint(SpawnLocation, SpawnRotation);
        SpawnLocation += SpawnRotation.Vector() * 150.f; // 카메라에서 150cm 앞

        FActorSpawnParameters Params;
        Params.Instigator = Character;
        Params.Owner      = Character;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        GetWorld()->SpawnActor<ALastFPSProjectile>(
            Weapon->ProjectileClass,
            SpawnLocation,
            SpawnRotation,
            Params);
    }

    Weapon->ConsumeAmmo();

    if (!Weapon->CanFire())
        EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
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
