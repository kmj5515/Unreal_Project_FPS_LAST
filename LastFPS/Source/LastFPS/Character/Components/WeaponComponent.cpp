#include "Character/Components/WeaponComponent.h"
#include "Weapons/LastFPSProjectile.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

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

        ApplyAnimLayer();
    }
}

void UWeaponComponent::ApplyAnimLayer()
{
    if (!WeaponAnimLayerClass)
        return;

    if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
        Owner->GetMesh()->LinkAnimClassLayers(WeaponAnimLayerClass);
}

void UWeaponComponent::RemoveAnimLayer()
{
    if (!WeaponAnimLayerClass)
        return;

    if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
        Owner->GetMesh()->UnlinkAnimClassLayers(WeaponAnimLayerClass);
}

void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner()->HasAuthority() || CurrentHeat <= 0.f)
        return;

    CurrentHeat = FMath::Max(0.f, CurrentHeat - CooldownRate * DeltaTime);

    if (bIsOverheated && CurrentHeat <= 0.f)
        bIsOverheated = false;

    // 서버에서 직접 브로드캐스트 (클라이언트는 OnRep_HeatState로 처리)
    OnHeatChanged.Broadcast(CurrentHeat, MaxHeat, bIsOverheated);
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

    OnHeatChanged.Broadcast(CurrentHeat, MaxHeat, bIsOverheated);
}

void UWeaponComponent::OnRep_HeatState()
{
    // 클라이언트: 복제된 값이 도착하면 HUD에 알림
    OnHeatChanged.Broadcast(CurrentHeat, MaxHeat, bIsOverheated);
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
