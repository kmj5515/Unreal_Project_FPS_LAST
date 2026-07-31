#include "Data/Projectiles/LastFPSAbilityProjectileData.h"

#include "Engine/World.h"
#include "Projectiles/LastFPSProjectile.h"

void ULastFPSAbilityProjectileData::CreateRenderWarmupActors(
    UWorld& World,
    const FTransform& SpawnTransform,
    TArray<AActor*>& OutActors) const
{
    UClass* LoadedProjectileClass = ProjectileClass.Get();
    if (!LoadedProjectileClass)
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParameters.ObjectFlags |= RF_Transient;
    SpawnParameters.bDeferConstruction = true;

    ALastFPSProjectile* WarmupActor = World.SpawnActor<ALastFPSProjectile>(
        LoadedProjectileClass,
        SpawnTransform,
        SpawnParameters);
    if (!WarmupActor)
    {
        return;
    }

    // 워밍업 인스턴스는 게임플레이나 네트워크 상태에 참여하지 않는다.
    WarmupActor->SetReplicates(false);
    WarmupActor->SetActorEnableCollision(false);
    WarmupActor->FinishSpawning(SpawnTransform);
    WarmupActor->SetActorHiddenInGame(true);
    WarmupActor->PrepareRenderWarmup(VisualData);
    OutActors.Add(WarmupActor);
}
