#include "Weapons/WeaponPickupActor.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"

AWeaponPickupActor::AWeaponPickupActor()
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = false;

    OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
    OverlapSphere->SetSphereRadius(PickupRadius);
    OverlapSphere->SetCollisionProfileName(TEXT("Trigger"));
    OverlapSphere->SetGenerateOverlapEvents(true);
    OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickupActor::OnOverlapBegin);
    RootComponent = OverlapSphere;

    PickupMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetCollisionProfileName(TEXT("NoCollision"));
    PickupMesh->SetupAttachment(RootComponent);
}

void AWeaponPickupActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (OverlapSphere)
    {
        OverlapSphere->SetSphereRadius(PickupRadius);
    }

    if (PickupMesh)
    {
        PickupMesh->SetSkeletalMesh(WeaponSkeletalMesh);
    }
}

void AWeaponPickupActor::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    OverlapSphere->SetSphereRadius(PickupRadius);
    PickupMesh->SetSkeletalMesh(WeaponSkeletalMesh);

    TArray<AActor*> OverlappingActors;
    OverlapSphere->GetOverlappingActors(OverlappingActors, ALastFPSHero::StaticClass());

    for (AActor* OverlappingActor : OverlappingActors)
    {
        TryEquipToActor(OverlappingActor);
        if (IsActorBeingDestroyed())
        {
            break;
        }
    }
}

void AWeaponPickupActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                         bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority())
    {
        return;
    }

    TryEquipToActor(OtherActor);
}

void AWeaponPickupActor::TryEquipToActor(AActor* OtherActor)
{
    ALastFPSHero* Hero = Cast<ALastFPSHero>(OtherActor);
    if (!Hero) return;

    UWeaponComponent* WeaponComp = Hero->GetWeaponComponent();
    if (!WeaponComp) return;
    WeaponComp->EquipWeapon(WeaponSkeletalMesh, WeaponType, WeaponAnimLayerClass, WeaponActorClass);
    Destroy();
}
