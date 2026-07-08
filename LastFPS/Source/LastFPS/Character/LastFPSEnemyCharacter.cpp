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
    UWorld* World = GetWorld();
    if (!DropPickupClass || !World)
    {
        return;
    }

    // 최종 스폰할 RowId 목록을 구성: (1) 확정분은 항상, (2) 남은 개수만 가중치 랜덤으로 채움.
    TArray<FName> SpawnRowIds;
    for (const FName& RowId : GuaranteedDropRowIds)
    {
        if (!RowId.IsNone())
        {
            SpawnRowIds.Add(RowId);
        }
    }

    const int32 RandomCount = FMath::Max(0, DropCount - SpawnRowIds.Num());
    if (RandomCount > 0)
    {
        // 유효(가중치>0) 후보의 가중치 합. 0이면 랜덤분은 건너뛴다(확정분은 그대로 나감).
        float TotalWeight = 0.f;
        for (const FLastFPSEnemyDropEntry& Entry : DropTable)
        {
            if (!Entry.ItemRowId.IsNone() && Entry.Weight > 0.f)
            {
                TotalWeight += Entry.Weight;
            }
        }
        if (TotalWeight > 0.f)
        {
            for (int32 i = 0; i < RandomCount; ++i)
            {
                const FName RowId = PickWeightedDropRowId(TotalWeight);
                if (!RowId.IsNone())
                {
                    SpawnRowIds.Add(RowId);
                }
            }
        }
    }

    const int32 Total = SpawnRowIds.Num();
    if (Total <= 0)
    {
        return;
    }

    // 사망 위치를 중심으로 원주에 균등 간격으로 동시 스폰.
    const FVector Center = GetActorLocation();
    for (int32 i = 0; i < Total; ++i)
    {
        const float AngleRad = (2.f * PI * i) / Total;
        const FVector Offset(FMath::Cos(AngleRad) * DropSpreadRadius,
                             FMath::Sin(AngleRad) * DropSpreadRadius, 0.f);
        const FTransform SpawnTransform(FRotator::ZeroRotator, Center + Offset);

        ALastFPSItemPickupActor* Pickup = World->SpawnActorDeferred<ALastFPSItemPickupActor>(
            DropPickupClass, SpawnTransform, this, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Pickup)
        {
            Pickup->ItemRowId = SpawnRowIds[i];
            Pickup->Count = 1;
            Pickup->FinishSpawning(SpawnTransform);
        }
    }
}

FName ALastFPSEnemyCharacter::PickWeightedDropRowId(float TotalWeight) const
{
    float Roll = FMath::FRandRange(0.f, TotalWeight);
    FName LastValid = NAME_None; // 부동소수 오차로 루프를 빠져나가는 경우의 폴백.
    for (const FLastFPSEnemyDropEntry& Entry : DropTable)
    {
        if (Entry.ItemRowId.IsNone() || Entry.Weight <= 0.f)
        {
            continue;
        }
        LastValid = Entry.ItemRowId;
        Roll -= Entry.Weight;
        if (Roll <= 0.f)
        {
            return Entry.ItemRowId;
        }
    }
    return LastValid;
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
