#include "Character/LastFPSEnemyCharacter.h"

#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Economy/LastFPSItemPickupActor.h"
#include "Engine/World.h"

ALastFPSEnemyCharacter::ALastFPSEnemyCharacter()
{
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ALastFPSEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        OnDeath.AddUObject(this, &ALastFPSEnemyCharacter::HandleOwnDeath);
    }
}

void ALastFPSEnemyCharacter::HandleOwnDeath(ALastFPSCharacterBase* /*DeadChar*/)
{
    if (!DropPickupClass || DropItemRowId.IsNone() || DropCount <= 0)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 사망 위치를 중심으로 DropCount 개를 원주에 균등 간격으로 동시 스폰.
    const FVector Center = GetActorLocation();
    for (int32 i = 0; i < DropCount; ++i)
    {
        const float AngleRad = (2.f * PI * i) / DropCount;
        const FVector Offset(FMath::Cos(AngleRad) * DropSpreadRadius,
                             FMath::Sin(AngleRad) * DropSpreadRadius, 0.f);
        const FTransform SpawnTransform(FRotator::ZeroRotator, Center + Offset);

        ALastFPSItemPickupActor* Pickup = World->SpawnActorDeferred<ALastFPSItemPickupActor>(
            DropPickupClass, SpawnTransform, this, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Pickup)
        {
            Pickup->ItemRowId = DropItemRowId;
            Pickup->Count = 1;
            Pickup->FinishSpawning(SpawnTransform);
        }
    }
}

void ALastFPSEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (!HasAuthority() || !AttributeSet)
    {
        return;
    }

    AttributeSet->SetMaxHealth(MaxHealth);
    AttributeSet->SetHealth(MaxHealth);
}
