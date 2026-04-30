#include "Character/Components/WeaponComponent.h"
#include "Weapons/LastFPSProjectile.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

UWeaponComponent::UWeaponComponent()
{
    SetIsReplicated(true);
    PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UWeaponComponent, CurrentAmmo);
}

void UWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    CurrentAmmo = MaxAmmo;

    // 오너 캐릭터의 스켈레탈 메시 소켓에 무기 부착
    if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
    {
        WeaponMesh = NewObject<USkeletalMeshComponent>(GetOwner(), TEXT("WeaponMesh"));
        WeaponMesh->RegisterComponent();
        WeaponMesh->AttachToComponent(Owner->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            AttachSocketName);
    }
}

bool UWeaponComponent::CanFire() const
{
    return CurrentAmmo > 0;
}

void UWeaponComponent::ConsumeAmmo()
{
    CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
}

FTransform UWeaponComponent::GetMuzzleTransform() const
{
    if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
        return WeaponMesh->GetSocketTransform(MuzzleSocketName);

    // MuzzleSocket 없으면 오너 위치 반환
    return GetOwner() ? GetOwner()->GetActorTransform() : FTransform::Identity;
}
