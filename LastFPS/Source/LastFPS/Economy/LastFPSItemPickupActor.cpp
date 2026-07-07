#include "Economy/LastFPSItemPickupActor.h"

#include "Character/LastFPSHero.h"
#include "Game/LastFPSPlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ALastFPSItemPickupActor::ALastFPSItemPickupActor()
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = false;

    OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
    OverlapSphere->SetSphereRadius(PickupRadius);
    OverlapSphere->SetCollisionProfileName(TEXT("Trigger"));
    OverlapSphere->SetGenerateOverlapEvents(true);
    OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ALastFPSItemPickupActor::OnOverlapBegin);
    RootComponent = OverlapSphere;

    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetCollisionProfileName(TEXT("NoCollision"));
    PickupMesh->SetupAttachment(RootComponent);
}

void ALastFPSItemPickupActor::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    OverlapSphere->SetSphereRadius(PickupRadius);

    // 스폰 순간 이미 겹쳐 있던 Hero 도 즉시 처리.
    TArray<AActor*> Overlapping;
    OverlapSphere->GetOverlappingActors(Overlapping, ALastFPSHero::StaticClass());
    for (AActor* Actor : Overlapping)
    {
        TryGrant(Actor);
        if (IsActorBeingDestroyed())
        {
            break;
        }
    }
}

void ALastFPSItemPickupActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                             bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority())
    {
        return;
    }

    TryGrant(OtherActor);
}

void ALastFPSItemPickupActor::TryGrant(AActor* OtherActor)
{
    ALastFPSHero* Hero = Cast<ALastFPSHero>(OtherActor);
    if (!Hero)
    {
        return;
    }

    // 지급은 주운 플레이어의 PlayerState 로 위임 — 그 플레이어 소유 클라의 로컬 Economy 에 들어간다.
    if (ALastFPSPlayerState* PS = Hero->GetPlayerState<ALastFPSPlayerState>())
    {
        PS->Auth_GrantItem(ItemRowId, Count);
    }

    Destroy();
}
