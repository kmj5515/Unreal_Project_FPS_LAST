#include "Weapons/LastFPSWeaponActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ALastFPSWeaponActor::ALastFPSWeaponActor()
{
    bReplicates = true;
    SetReplicateMovement(true);
    PrimaryActorTick.bCanEverTick = true;

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
    WeaponMesh->SetGenerateOverlapEvents(false);
    RootComponent = WeaponMesh;
}

void ALastFPSWeaponActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // 컴포넌트 초기화 이후에 호출해야 한다. 생성자에서 SetIsReplicated를 부르면
    // "SetIsReplicatedByDefault is preferred during Component Construction" ensure가 발동한다.
    if (WeaponMesh)
    {
        WeaponMesh->SetIsReplicated(true);
    }
}

void ALastFPSWeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSWeaponActor, WeaponMeshAsset);
}

void ALastFPSWeaponActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bDrawProjectileStartDebug || !WeaponMesh || !WeaponMesh->DoesSocketExist(DebugProjectileStartSocketName))
    {
        return;
    }

    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn && !OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FTransform SocketTransform = WeaponMesh->GetSocketTransform(DebugProjectileStartSocketName, RTS_World);
    const FVector SocketLocation = SocketTransform.GetLocation();
    const FQuat SocketRotation = SocketTransform.GetRotation();

    DrawDebugSphere(World, SocketLocation, DebugSphereRadius, 12, FColor::Cyan, false, 0.f, 0, 1.5f);
    DrawDebugLine(World, SocketLocation, SocketLocation + SocketRotation.GetForwardVector() * DebugAxisLength, FColor::Red, false, 0.f, 0, 2.f);
    DrawDebugLine(World, SocketLocation, SocketLocation + SocketRotation.GetRightVector() * DebugAxisLength, FColor::Green, false, 0.f, 0, 2.f);
    DrawDebugLine(World, SocketLocation, SocketLocation + SocketRotation.GetUpVector() * DebugAxisLength, FColor::Blue, false, 0.f, 0, 2.f);
}

void ALastFPSWeaponActor::InitializeWeapon(USkeletalMesh* InMesh, UParticleSystem* InMuzzleFlash, USoundBase* InFireSound)
{
    WeaponMeshAsset = InMesh;
    OnRep_WeaponMeshAsset();

    MuzzleFlashEffect = InMuzzleFlash;
    FireSound = InFireSound;
}

void ALastFPSWeaponActor::OnRep_WeaponMeshAsset()
{
    if (WeaponMesh)
    {
        WeaponMesh->SetSkeletalMesh(WeaponMeshAsset);
    }
}

FTransform ALastFPSWeaponActor::GetMuzzleTransform(FName MuzzleSocketName) const
{
    if (!WeaponMesh)
    {
        return GetActorTransform();
    }

    if (!WeaponMesh->DoesSocketExist(MuzzleSocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("WeaponActor muzzle socket missing: Actor=%s Socket=%s Mesh=%s"),
            *GetName(),
            *MuzzleSocketName.ToString(),
            WeaponMesh->GetSkeletalMeshAsset() ? *WeaponMesh->GetSkeletalMeshAsset()->GetName() : TEXT("None"));
        return WeaponMesh->GetComponentTransform();
    }

    return WeaponMesh->GetSocketTransform(MuzzleSocketName, RTS_World);
}

bool ALastFPSWeaponActor::GetSocketTransformInBoneSpace(FName SocketName, USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const
{
    OutTransform = FTransform::Identity;

    if (!WeaponMesh || !CharacterMesh || !WeaponMesh->DoesSocketExist(SocketName))
    {
        return false;
    }

    const FTransform SocketWorldTransform = WeaponMesh->GetSocketTransform(SocketName, RTS_World);

    FVector BoneSpaceLocation = FVector::ZeroVector;
    FRotator BoneSpaceRotation = FRotator::ZeroRotator;
    CharacterMesh->TransformToBoneSpace(
        RelativeToBoneName,
        SocketWorldTransform.GetLocation(),
        SocketWorldTransform.Rotator(),
        BoneSpaceLocation,
        BoneSpaceRotation);

    OutTransform = FTransform(BoneSpaceRotation, BoneSpaceLocation, SocketWorldTransform.GetScale3D());
    return true;
}

void ALastFPSWeaponActor::PlayFireEffects(FName MuzzleSocketName) const
{
    if (!WeaponMesh)
    {
        return;
    }

    if (FireSound)
    {
        UGameplayStatics::SpawnSoundAttached(FireSound, WeaponMesh, MuzzleSocketName);
    }

    if (MuzzleFlashEffect)
    {
        UGameplayStatics::SpawnEmitterAttached(
            MuzzleFlashEffect,
            WeaponMesh,
            MuzzleSocketName,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget);
    }
}
