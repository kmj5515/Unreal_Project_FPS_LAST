#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/LastFPSItemData.h"
#include "LastFPSItemSlotWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * WBP_ItemSlot 의 Parent — 인벤토리 그리드 슬롯 1칸.
 * Designer: Image_Icon / TB_ItemName / TB_Rarity (모두 선택).
 * 희귀도 색상 등 스타일링은 OnSlotDisplayed(BP 이벤트)에서.
 * 비어있는 슬롯은 SetEmpty()로 처리(고정 슬롯 그리드 유지).
 */
UCLASS()
class LASTFPS_API ULastFPSItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 아이템 데이터로 슬롯 채우기 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Inventory")
	void SetupSlot(const FLastFPSItemData& InItem);

	/** 빈 슬롯으로 초기화 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Inventory")
	void SetEmpty();

protected:
	/** 희귀도 색상·테두리 등 BP 측 스타일링 훅 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Inventory")
	void OnSlotDisplayed(ELastFPSItemType ItemType, ELastFPSItemRarity Rarity);

	/** 빈 슬롯 전환 시 BP 훅 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Inventory")
	void OnSlotCleared();

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_ItemName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Rarity;
};
