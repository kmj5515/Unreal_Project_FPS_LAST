#include "Character/Components/WeaponComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UWeaponComponent::UWeaponComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
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

    // 초기에는 열이 없으므로 Tick 꺼둠 — AddHeat 호출 시 켜짐
    SetComponentTickEnabled(false);

    if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
    {
        if (WeaponSkeletalMesh)
            WeaponMesh->SetSkeletalMesh(WeaponSkeletalMesh);

        WeaponMesh->AttachToComponent(Owner->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);

        ApplyAnimLayer();
        OnWeaponEquippedChanged.Broadcast(WeaponSkeletalMesh != nullptr);
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

    if (!GetOwner()->HasAuthority()) return;

    CurrentHeat = FMath::Max(0.f, CurrentHeat - CooldownRate * DeltaTime);

    if (bIsOverheated && CurrentHeat <= 0.f)
        bIsOverheated = false;

    // 서버에서 직접 브로드캐스트 (클라이언트는 OnRep_HeatState로 처리)
    OnHeatChanged.Broadcast(CurrentHeat, MaxHeat, bIsOverheated);

    // 완전히 냉각되면 Tick 중단 — 다음 발사 시 AddHeat에서 다시 켜짐
    if (CurrentHeat <= 0.f)
        SetComponentTickEnabled(false);
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

    SetComponentTickEnabled(true);
    OnHeatChanged.Broadcast(CurrentHeat, MaxHeat, bIsOverheated);
}

void UWeaponComponent::OnRep_HeatState()
{
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
