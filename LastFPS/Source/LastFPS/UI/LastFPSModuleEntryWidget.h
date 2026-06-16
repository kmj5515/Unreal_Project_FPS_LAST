#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/LastFPSItemData.h"
#include "LastFPSModuleEntryWidget.generated.h"

class UTextBlock;
class UImage;
class ULastFPSButtonBase;

/** 장착 버튼 클릭 시 부모(모듈 화면)에 모듈 행 이름 전달 */
DECLARE_DELEGATE_OneParam(FOnModuleEquipClicked, FName /*RowName*/);

/**
 * WBP_ModuleEntry 의 Parent — 보유 모듈 목록의 한 줄.
 * 표시(아이콘/이름/희귀도)는 DT_ItemData 기준, 스탯/캐파 텍스트는 화면이 포맷해 넘긴다.
 * Designer 바인딩(모두 선택): Image_Icon / Img_RarityBorder / TB_ModuleName / TB_Stats / TB_Capacity / TB_Count / Button_Equip
 */
UCLASS()
class LASTFPS_API ULastFPSModuleEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @param StatText 화면이 포맷한 스탯 요약(예: "공격력 +25"). @param CapacityCost 캐파 코스트. */
	void SetupModule(const FLastFPSItemData& InItem, const FText& StatText, int32 CapacityCost, int32 Count, FName InRowName);

	/** 장착 가능 여부(빈 슬롯·캐파)에 따라 장착 버튼 활성/비활성 */
	void SetEquipEnabled(bool bEnabled);

	FOnModuleEquipClicked OnEquipClicked;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Image_Icon;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Img_RarityBorder;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_ModuleName;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Stats;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Capacity;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Count;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<ULastFPSButtonBase> Button_Equip;

private:
	void HandleEquipClicked();
	FName RowName;
};
