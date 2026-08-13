#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "LastFPSDropProfile.generated.h"

class ALastFPSItemPickupActor;

USTRUCT(BlueprintType)
struct FLastFPSDropEntry
{
    GENERATED_BODY()

    /** 스폰할 픽업. 무엇을 지급하는지는 이 픽업 BP 의 값이 정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drop")
    TSubclassOf<ALastFPSItemPickupActor> PickupClass;

    /** 아이템 지급 픽업일 때 넘길 DT_ItemData 행. 체력·탄약 픽업은 비워 둔다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drop")
    FName ItemRowId;

    /** 드랍 확률 0~1. 1 이면 확정 드랍. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drop", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Chance = 1.f;

    /** 픽업 1개가 지급할 수량. 아이템 픽업에서만 의미가 있다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drop", meta=(ClampMin="1"))
    int32 Count = 1;
};

/**
 * 전투 레벨 1개의 처치 드랍 규칙.
 * 보스와 잡몹 두 갈래로만 나눈다 — 등급이 더 필요해지면 분류 태그 기반으로 일반화한다.
 */
UCLASS(BlueprintType, Const)
class LASTFPS_API ULastFPSDropProfile : public UDataAsset
{
    GENERATED_BODY()

public:
    /** 잡몹 기본 드랍. 체력·탄약 보급이 여기 들어간다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Drop")
    TArray<FLastFPSDropEntry> CommonDrops;

    /** 보스 드랍. 아이템 보상이 여기 들어간다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Drop")
    TArray<FLastFPSDropEntry> BossDrops;

    /** 픽업을 뿌릴 원 반경. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Drop", meta=(ClampMin="0.0", Units="cm"))
    float DropSpreadRadius = 150.f;

    void RollDrops(bool bIsBoss, TArray<FLastFPSDropEntry>& OutEntries) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
