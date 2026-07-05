#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSItemData.h"
#include "LastFPSItemSlotWidget.generated.h"

class UTextBlock;
class UImage;
class ULastFPSItemTooltipWidget;

/** 슬롯에 마우스가 들어오거나 나갈 때 부모(인벤토리)에 아이템 RowId 전달 — hover 정보/F1 프리뷰 추적용. */
DECLARE_DELEGATE_OneParam(FOnItemSlotHovered, FName /*RowId*/);

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
	/** @param InRowId DT_ItemData 행 이름(툴팁 모듈 스탯 조회·F1 프리뷰용). @param Count 보유 수량(1 이하면 수량 텍스트 숨김). */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Inventory")
	void SetupSlot(const FLastFPSItemData& InItem, FName InRowId, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Inventory")
	void SetEmpty();

	static FLinearColor RarityToColor(ELastFPSItemRarity Rarity);

	/** 이 슬롯이 표현하는 아이템 행 이름 (없으면 NAME_None) */
	FName GetItemRowId() const { return ItemRowId; }

	/** 마우스 진입/이탈 알림 (부모가 바인딩) */
	FOnItemSlotHovered OnHovered;
	FOnItemSlotHovered OnUnhovered;

protected:
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/** hover 시 뜰 툴팁 위젯 클래스 (WBP_ItemTooltip). 미지정 시 툴팁 없음. */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|Inventory")
	TSubclassOf<ULastFPSItemTooltipWidget> TooltipWidgetClass;
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

private:
	/** 이 슬롯이 표현하는 아이템 행 이름 */
	FName ItemRowId;
};
