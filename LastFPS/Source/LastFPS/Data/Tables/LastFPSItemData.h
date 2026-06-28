#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LastFPSItemData.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ELastFPSItemType : uint8
{
	Weapon		UMETA(DisplayName="무기"),
	Module		UMETA(DisplayName="모듈"),
	Consumable	UMETA(DisplayName="소모품"),
	Material	UMETA(DisplayName="재료"),
};

UENUM(BlueprintType)
enum class ELastFPSItemRarity : uint8
{
	Common		UMETA(DisplayName="일반"),
	Rare		UMETA(DisplayName="희귀"),
	Epic		UMETA(DisplayName="영웅"),
	Legendary	UMETA(DisplayName="전설"),
};

/**
 * 아이템 1종의 정의 — DataTable 행.
 * 인벤토리 UI가 읽는 정의(메타데이터)이며, 런타임 보유량/장착 상태는 별도 서브시스템에서 관리(추후).
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	ELastFPSItemType ItemType = ELastFPSItemType::Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	ELastFPSItemRarity Rarity = ELastFPSItemRarity::Common;

	/** 슬롯 1칸에 최대 겹치는 수량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	int32 MaxStackSize = 1;
};
