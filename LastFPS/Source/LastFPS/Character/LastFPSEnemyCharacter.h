#pragma once

#include "CoreMinimal.h"
#include "Character/LastFPSCharacterBase.h"
#include "LastFPSEnemyCharacter.generated.h"

class ALastFPSItemPickupActor;

UCLASS()
class LASTFPS_API ALastFPSEnemyCharacter : public ALastFPSCharacterBase
{
    GENERATED_BODY()

public:
    ALastFPSEnemyCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Stats")
    float MaxHealth = 10000.f;

    // 사망 시 드랍할 픽업 (비우면 드랍 없음).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    TSubclassOf<ALastFPSItemPickupActor> DropPickupClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    FName DropItemRowId;

    // 사망 위치 원주에 균등 배치할 드랍 픽업 개수. 각 픽업은 1개씩 지급.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    int32 DropCount = 1;

    // 드랍을 뿌릴 원 반경 (cm).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    float DropSpreadRadius = 150.f;

private:
    void HandleOwnDeath(ALastFPSCharacterBase* DeadChar);
};
