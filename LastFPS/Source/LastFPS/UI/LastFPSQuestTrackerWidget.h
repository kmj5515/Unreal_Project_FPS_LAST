#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSQuestTrackerWidget.generated.h"

class UDataTable;
class UPanelWidget;
class UTextBlock;
class ULastFPSQuestEntryWidget;

/**
 * HUD 퀘스트 트래커 — 게임 화면 한쪽에 "진행중" 퀘스트만 간략히 나열.
 * QuestTable(DT_QuestData)에서 Status == InProgress 인 행만 골라 EntryWidgetClass로 표시.
 * 진행 추적 서브시스템은 아직 없음 — 행의 Status 값을 그대로 필터링(프로토).
 */
UCLASS()
class LASTFPS_API ULastFPSQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	/** 퀘스트 정의 테이블 (RowType = FLastFPSQuestData) — 임무 화면과 동일 테이블 재사용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(RequiredAssetDataTags="RowStructure=/Script/LastFPS.LastFPSQuestData"))
	TObjectPtr<UDataTable> QuestTable;

	/** 행 하나를 그릴 위젯 클래스 (퀘스트 화면과 동일한 WBP_QuestEntry 재사용 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSubclassOf<ULastFPSQuestEntryWidget> EntryWidgetClass;

	/** 표시할 최대 진행중 퀘스트 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin="1"))
	int32 MaxTrackedQuests = 3;

	/** 엔트리를 담을 컨테이너 (VerticalBox 등) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_TrackerList;

	/** 진행중 퀘스트가 하나도 없을 때 보일 안내 텍스트 (선택) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Empty;

	/** 테이블을 다시 읽어 진행중 퀘스트 목록을 재구성 */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void RebuildTracker();
};
