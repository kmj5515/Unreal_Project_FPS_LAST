#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSItemData.h"
#include "LastFPSItemTooltipWidget.generated.h"

class UTextBlock;

/**
 * WBP_ItemTooltip 의 Parent — 아이템 hover 시 뜨는 간략 정보 팝업.
 * 인벤토리/소모품 슬롯이 공유한다. 모듈이면 DT_ModuleData 의 스탯 보정을 요약 표시,
 * 무기/소모품 등은 이름 + 희귀도 + 설명만(스탯 줄은 숨김).
 *
 * Designer 바인딩: TB_ItemName / TB_Rarity / TB_Description / TB_Stats
 */
UCLASS()
class LASTFPS_API ULastFPSItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 아이템 정보로 툴팁을 채운다. InRowId 는 모듈 스탯 조회용(모듈이 아니면 무시). */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Tooltip")
	void SetupTooltip(const FLastFPSItemData& InItem, FName InRowId);

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_ItemName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Rarity;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Description;

	/** 모듈 스탯 보정 요약 (예: "공격력 +10"). 스탯이 없으면 숨김. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Stats;

	/** "상세 보기(F1)" 힌트 — F1 상세 프리뷰가 있는 아이템(무기+WeaponDefinition)에만 표시. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Detail;
};
