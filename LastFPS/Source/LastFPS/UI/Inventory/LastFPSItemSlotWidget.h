#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSItemData.h"
#include "LastFPSItemSlotWidget.generated.h"

class UTextBlock;
class UImage;
class UWidget;
class ULastFPSItemTooltipWidget;
struct FStreamableHandle;

/** 슬롯에 마우스가 들어오거나 나갈 때 부모(인벤토리)에 아이템 RowId 전달 — hover 정보/F1 프리뷰 추적용. */
DECLARE_DELEGATE_OneParam(FOnItemSlotHovered, FName /*RowId*/);

/** 슬롯 클릭 신호. 바인딩한 화면에서만 선택·확정 동작으로 해석한다. */
DECLARE_DELEGATE_OneParam(FOnItemSlotInteraction, FName /*RowId*/);

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

	/** 선택 화면에서 사용할 강조 표시. 일반 인벤토리에서는 호출하지 않는다. */
	void SetSelected(bool bInSelected);

	/** 마우스 진입/이탈 알림 (부모가 바인딩) */
	FOnItemSlotHovered OnHovered;
	FOnItemSlotHovered OnUnhovered;
	FOnItemSlotInteraction OnClicked;
	FOnItemSlotInteraction OnDoubleClicked;

protected:
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(
		const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/** hover 시 뜰 툴팁 위젯 클래스 (WBP_ItemTooltip). 미지정 시 툴팁 없음. */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|Inventory")
	TSubclassOf<ULastFPSItemTooltipWidget> TooltipWidgetClass;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_Background;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_RarityBorder;

	/** 선택 화면에서만 사용하는 강조 테두리. 기존 WBP_ItemSlot에는 없어도 동작한다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_SelectionBorder;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_ItemName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Rarity;

	/** 보유 수량 (예: "x3"). 1개면 숨김. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Count;

	virtual void NativeDestruct() override;

private:
	/**
	 * 호버 시점에 툴팁을 만든다.
	 *
	 * 슬롯마다 미리 만들어 두면 인벤토리를 여는 순간 아이템 수만큼 위젯 트리가 늘어난다.
	 * 실제로 보이는 것은 한 번에 하나뿐이라 만들 이유가 없다.
	 */
	UFUNCTION()
	UWidget* CreateItemTooltip();

	/** 이 슬롯이 표현하는 아이템 행 이름 */
	FName ItemRowId;

	/** 툴팁을 나중에 만들기 위해 들고 있는 표시 데이터. 행 조회를 다시 하지 않으려고 복사해 둔다. */
	FLastFPSItemData CachedItem;

	/** 진행 중인 아이콘 로드. 슬롯이 다른 아이템으로 재사용될 때 취소한다. */
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
