#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Inventory/LastFPSItemData.h"
#include "LastFPSShopData.generated.h"

class UTexture2D;

/**
 * 상점 판매 항목 1건의 데이터 — DataTable 행.
 * 상점 화면(WBP_Shop)이 모든 행을 읽어 나열한다. 화폐/재고 시스템은 아직 없고,
 * 가격 표시 + 구매 버튼(프로토: 구매 시 표시만 "구매됨"으로 전환)까지만 처리.
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSShopItemData : public FTableRowBase
{
	GENERATED_BODY()

	/** 목록에 표시할 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
	FText ItemName;

	/** 상세 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(MultiLine=true))
	FText Description;

	/** 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 희귀도 — 아이콘 테두리/텍스트 색상에 사용 (인벤토리와 동일 컬러 규칙) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
	ELastFPSItemRarity Rarity = ELastFPSItemRarity::Common;

	/** 가격 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ClampMin=0))
	int32 Price = 0;

	/** 구매 시 인벤토리에 지급할 아이템 — DT_ItemData 의 행 이름. 비우면 화폐만 차감. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
	FName GrantItemRowId;
};
