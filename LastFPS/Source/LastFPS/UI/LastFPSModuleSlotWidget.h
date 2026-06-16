#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/LastFPSItemData.h"
#include "LastFPSModuleSlotWidget.generated.h"

class UTextBlock;
class UImage;
class ULastFPSButtonBase;

/** 슬롯 클릭 시 부모(모듈 화면)에 슬롯 인덱스 전달 (해제용) */
DECLARE_DELEGATE_OneParam(FOnModuleSlotClicked, int32 /*SlotIndex*/);

/**
 * WBP_ModuleSlot 의 Parent — 로드아웃 장착 슬롯 1칸. 장착 중 클릭 시 해제.
 * Designer 바인딩(모두 선택): Image_Icon / Img_RarityBorder / Img_Empty / TB_ModuleName / TB_Stats / Button_Slot
 */
UCLASS()
class LASTFPS_API ULastFPSModuleSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @param StatText 화면이 포맷한 스탯 요약. */
	void SetupEquipped(int32 InSlotIndex, const FLastFPSItemData& InItem, const FText& StatText);
	void SetEmpty(int32 InSlotIndex);

	FOnModuleSlotClicked OnSlotClicked;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Image_Icon;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Img_RarityBorder;
	/** 빈 슬롯 표시용 (장착 시 숨김) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Img_Empty;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_ModuleName;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Stats;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<ULastFPSButtonBase> Button_Slot;

private:
	void HandleSlotClicked();
	int32 SlotIndex = INDEX_NONE;
};
