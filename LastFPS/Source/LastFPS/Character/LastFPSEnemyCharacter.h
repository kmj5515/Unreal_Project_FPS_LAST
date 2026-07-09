#pragma once

#include "CoreMinimal.h"
#include "Character/LastFPSCharacterBase.h"
#include "LastFPSEnemyCharacter.generated.h"

class ALastFPSItemPickupActor;

/** 가중치 기반 드랍 항목 1종. Weight 가 클수록 자주 뽑힌다. */
USTRUCT(BlueprintType)
struct FLastFPSEnemyDropEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drop")
    FName ItemRowId;

    // 추첨 가중치 (상대값). 0 이하면 후보에서 제외.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drop", meta=(ClampMin="0.0"))
    float Weight = 1.f;
};

UCLASS()
class LASTFPS_API ALastFPSEnemyCharacter : public ALastFPSCharacterBase
{
    GENERATED_BODY()

public:
    ALastFPSEnemyCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;
    
    // 사망 시 드랍할 픽업 (비우면 드랍 없음).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    TSubclassOf<ALastFPSItemPickupActor> DropPickupClass;

    // 확정 드랍. 여기 나열한 RowId 는 항상 1개씩 나온다(같은 걸 여러 번 나열하면 그 수만큼). 총량(DropCount)에 포함.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    TArray<FName> GuaranteedDropRowIds;

    // 확정분을 채우고 남은 개수를 채울 후보(가중치 랜덤). 비면 랜덤분 없음.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    TArray<FLastFPSEnemyDropEntry> DropTable;

    // 총 드랍 픽업 개수(확정 + 랜덤). 확정분이 이보다 많으면 확정분은 모두 나온다. 각 픽업은 1개씩 지급.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    int32 DropCount = 1;

    // 드랍을 뿌릴 원 반경 (cm).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    float DropSpreadRadius = 150.f;

private:
    void HandleOwnDeath(ALastFPSCharacterBase* DeadChar);

    // DropTable 에서 가중치로 RowId 1개 추첨. TotalWeight 는 유효 항목 가중치 합. 실패 시 NAME_None.
    FName PickWeightedDropRowId(float TotalWeight) const;
};
