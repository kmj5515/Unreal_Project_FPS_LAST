#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "LastFPSQuestEntryWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * WBP_QuestEntry 의 Parent — 퀘스트 목록의 한 줄.
 * Designer: TB_Title / TB_Summary / TB_Status / TB_Reward / Img_Icon (모두 선택).
 * 상태별 색상 등 스타일링은 OnQuestDisplayed(BP 이벤트)에서.
 */
UCLASS()
class LASTFPS_API ULastFPSQuestEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 행 데이터로 표시 내용 채우기 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void SetupQuest(const FLastFPSQuestData& InQuest);

protected:
	/** 상태 텍스트/색상 등 BP 측 스타일링 훅 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Quest")
	void OnQuestDisplayed(ELastFPSQuestType Type, ELastFPSQuestStatus Status);

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Title;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Summary;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Status;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Reward;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon;

public:
	/** 상태 enum → 표시 텍스트 (목록/HUD 공용) */
	static FText StatusToText(ELastFPSQuestStatus Status);
};
