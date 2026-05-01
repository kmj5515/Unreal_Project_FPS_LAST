#include "Character/Components/WeaponComponent.h"
#include "Weapons/LastFPSProjectile.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UWeaponComponent::UWeaponComponent()
{
    SetIsReplicated(true);
    PrimaryComponentTick.bCanEverTick = true;
}

void UWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UWeaponComponent, CurrentHeat);
    DOREPLIFETIME(UWeaponComponent, bIsOverheated);
}

void UWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
    {
        WeaponMesh = NewObject<USkeletalMeshComponent>(GetOwner(), TEXT("WeaponMesh"));
        WeaponMesh->RegisterComponent();

        if (WeaponSkeletalMesh)
            WeaponMesh->SetSkeletalMesh(WeaponSkeletalMesh);

        WeaponMesh->AttachToComponent(Owner->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            AttachSocketName);
    }
}

void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 열 감소는 서버에서만 처리 — 클라이언트는 Replicated 값으로 동기화
    if (!GetOwner()->HasAuthority() || CurrentHeat <= 0.f)
        return;

    CurrentHeat = FMath::Max(0.f, CurrentHeat - CooldownRate * DeltaTime);

    // 오버히트 상태에서 완전히 냉각되면 해제
    if (bIsOverheated && CurrentHeat <= 0.f)
        bIsOverheated = false;
}

bool UWeaponComponent::CanFire() const
{
    return !bIsOverheated;
}

void UWeaponComponent::AddHeat()
{
    if (!GetOwner()->HasAuthority())
        return;

    CurrentHeat = FMath::Min(MaxHeat, CurrentHeat + HeatPerShot);

    if (CurrentHeat >= MaxHeat)
        bIsOverheated = true;
}

void UWeaponComponent::PlayFireEffects() const
{
    if (!WeaponMesh)
        return;

    if (FireSound)
        UGameplayStatics::SpawnSoundAttached(FireSound, WeaponMesh, MuzzleSocketName);

    if (MuzzleFlashEffect)
        UGameplayStatics::SpawnEmitterAttached(
            MuzzleFlashEffect, WeaponMesh, MuzzleSocketName,
            FVector::ZeroVector, FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget);
}

FTransform UWeaponComponent::GetMuzzleTransform() const
{
    if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
        return WeaponMesh->GetSocketTransform(MuzzleSocketName);

    return GetOwner() ? GetOwner()->GetActorTransform() : FTransform::Identity;
}
