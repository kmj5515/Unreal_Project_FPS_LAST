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
    RootComponent = OverlapSphere;

    PickupMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetCollisionProfileName(TEXT("NoCollision"));
    PickupMesh->SetupAttachment(RootComponent);
}

void AWeaponPickupActor::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickupActor::OnOverlapBegin);
    }
}

void AWeaponPickupActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                         bool bFromSweep, const FHitResult& SweepResult)
{
    ALastFPSHero* Hero = Cast<ALastFPSHero>(OtherActor);
    if (!Hero) return;

    UWeaponComponent* WeaponComp = Hero->GetWeaponComponent();
    if (!WeaponComp) return;

    WeaponComp->EquipWeapon(WeaponSkeletalMesh, WeaponType, WeaponAnimLayerClass);
    Destroy();
}
