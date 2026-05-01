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
        // 총구 소켓 위치 + 카메라 조준 방향으로 발사
        // 총구에서 나오되, 크로스헤어(카메라 중앙) 방향을 향함
        FVector  CameraLocation;
        FRotator AimRotation;
        Character->GetController()->GetPlayerViewPoint(CameraLocation, AimRotation);

        FVector MuzzleLocation = Weapon->GetMuzzleTransform().GetLocation();

        FActorSpawnParameters Params;
        Params.Instigator = Character;
        Params.Owner      = Character;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        GetWorld()->SpawnActor<ALastFPSProjectile>(
            Weapon->ProjectileClass,
            MuzzleLocation,
            AimRotation,
            Params);
    }

    Weapon->AddHeat();

    // 오버히트 도달 시 어빌리티 종료 → 연사 타이머 자동 정지
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
