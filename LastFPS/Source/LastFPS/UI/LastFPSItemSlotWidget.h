#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/LastFPSItemData.h"
#include "LastFPSItemSlotWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * WBP_ItemSlot 의 Parent — 인벤토리 그리드 슬롯 1칸.
 * Designer 바인딩:
 *   Img_Background  — 빈 슬롯 배경 (아이템 있으면 Hidden)
 *   Img_RarityBorder — 희귀도 테두리/배경 (아이템 없으면 Hidden)
 *   Image_Icon      — 아이템 아이콘
 *   TB_ItemName     — 아이템 이름
 *   TB_Rarity       — 희귀도 텍스트
 */
UCLASS()
class LASTFPS_API ULastFPSItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @param Count 보유 수량. 1 이하면 수량 텍스트(TB_Count) 숨김. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Inventory")
	void SetupSlot(const FLastFPSItemData& InItem, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Inventory")
	void SetEmpty();

	static FLinearColor RarityToColor(ELastFPSItemRarity Rarity);

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_Background;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_RarityBorder;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_ItemName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Rarity;

	/** 보유 수량 (예: "x3"). 1개면 숨김. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Count;
};
