#include "AbilitySystem/Abilities/GA_BasicShoot.h"
#include "Character/Components/WeaponComponent.h"
#include "Character/LastFPSHero.h"
#include "Character/LastFPSCharacterBase.h"
#include "Weapons/LastFPSProjectile.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

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
        UE_LOG(LogTemp, Warning, TEXT("GA_BasicShoot: 사망 상태에서 발사 시도됨"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_BasicShoot: CommitAbility 실패 (쿨다운/비용)"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CachedWeapon = GetWeaponComponent();
    UWeaponComponent* Weapon = CachedWeapon.Get();
    if (!Weapon || !Weapon->CanFire())
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_BasicShoot: 무기 없음 또는 오버히트 상태"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Fire();

    if (bIsAutoFire)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                FireTimerHandle,
                this, &UGA_BasicShoot::Fire,
                Weapon->FireRate, true);
        }
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UGA_BasicShoot::Fire()
{
    if (!GetWorld()) return;

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

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character) return;

    // 로컬 클라이언트: 사운드 + 머즐플래시 즉시 재생 (레이턴시 없는 클라이언트 예측)
    if (Character->IsLocallyControlled())
        LocalFire(Weapon);

    // 서버: LineTrace 히트 판정 + 데미지 GE 적용 + VFX 투사체 스폰
    if (Character->HasAuthority())
        ServerFire(Character, Weapon);

    Weapon->AddHeat();

    if (!Weapon->CanFire())
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_BasicShoot::LocalFire(UWeaponComponent* Weapon)
{
    Weapon->PlayFireEffects();
}

void UGA_BasicShoot::ServerFire(ACharacter* Character, UWeaponComponent* Weapon)
{
    UWorld* World = GetWorld();
    if (!World) return;

    AController* Controller = Character->GetController();
    if (!Controller) return;

    FVector  CameraLocation;
    FRotator AimRotation;
    Controller->GetPlayerViewPoint(CameraLocation, AimRotation);

    const FVector MuzzleLocation = Weapon->GetMuzzleTransform().GetLocation();
    const FVector TraceEnd       = CameraLocation + AimRotation.Vector() * 10000.f;

    FHitResult HitResult;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponTrace), false, Character);
    QueryParams.AddIgnoredActor(Character);

    // ECC_Visibility만으로는 캐릭터 캡슐을 인식 못할 수 있으므로
    // WorldStatic/Dynamic + Pawn 오브젝트 타입을 모두 포함해 트레이스
    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

    const bool bHit = World->LineTraceSingleByObjectType(
        HitResult, CameraLocation, TraceEnd, ObjectParams, QueryParams);

    // VFX 전용 투사체 스폰 (서버 스폰 → bReplicates로 모든 클라이언트에 복제됨)
    if (Weapon->ProjectileClass)
    {
        const FVector  AimTarget          = bHit ? HitResult.ImpactPoint : TraceEnd;
        const FRotator ProjectileRotation = (AimTarget - MuzzleLocation).Rotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Instigator = Character;
        SpawnParams.Owner      = Character;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        World->SpawnActor<ALastFPSProjectile>(
            Weapon->ProjectileClass, MuzzleLocation, ProjectileRotation, SpawnParams);
    }

    if (!bHit || !HitResult.GetActor() || !DamageEffectClass) return;

    IAbilitySystemInterface* SourceASI = Cast<IAbilitySystemInterface>(Character);
    IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(HitResult.GetActor());
    if (!SourceASI || !TargetASI) return;

    UAbilitySystemComponent* SourceASC = SourceASI->GetAbilitySystemComponent();
    UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
    if (!SourceASC || !TargetASC) return;

    FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
    Context.AddSourceObject(Character);
    if (APawn* Pawn = Cast<APawn>(Character))
        Context.AddInstigator(Pawn, Character);

    FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
    if (Spec.IsValid())
    {
        TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

        if (ALastFPSCharacterBase* ShooterChar = Cast<ALastFPSCharacterBase>(Character))
            ShooterChar->Client_NotifyHitMarker();
    }
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
