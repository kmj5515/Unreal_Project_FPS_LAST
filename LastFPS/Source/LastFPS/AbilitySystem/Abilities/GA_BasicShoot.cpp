#include "AbilitySystem/Abilities/GA_BasicShoot.h"

#include "Character/Components/WeaponComponent.h"
#include "Character/LastFPSHero.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Utility/LastFPSTags.h"

UGA_BasicShoot::UGA_BasicShoot()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    const FLastFPSTags& FPSTags = FLastFPSTags::Get();
    FGameplayTagContainer Tags;
    Tags.AddTag(FPSTags.Ability_Fire);
    Tags.AddTag(FPSTags.Input_Fire);
    SetAssetTags(Tags);
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
        UE_LOG(LogTemp, Warning, TEXT("GA_BasicShoot: cannot fire while dead or missing hero"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
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

    Fire();

    if (bIsAutoFire)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                FireTimerHandle,
                this,
                &UGA_BasicShoot::Fire,
                Weapon->FireRate,
                true);
        }
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UGA_BasicShoot::Fire()
{
    if (!GetWorld())
    {
        return;
    }

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
    if (!Character || !Character->IsLocallyControlled())
    {
        return;
    }

    LocalFire(Weapon);

    AController* Controller = Character->GetController();
    if (!Controller)
{
        return;
    }

    FVector CameraLocation;
    FRotator AimRotation;
    Controller->GetPlayerViewPoint(CameraLocation, AimRotation);

    Weapon->FireFromClientAim(
        Weapon->GetMuzzleTransform().GetLocation(),
        CameraLocation,
        AimRotation.Vector(),
        DamageEffectClass,
        bDrawDebugShot,
        DebugShotDuration);

    if (!Weapon->CanFire())
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
    }
}

void UGA_BasicShoot::LocalFire(UWeaponComponent* Weapon)
{
    if (Weapon)
    {
        Weapon->PlayFireEffects();
    }

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->GetMesh())
    {
        return;
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
