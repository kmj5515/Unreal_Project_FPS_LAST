#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSQuestTrackerWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class ULastFPSQuestEntryWidget;

/**
 * HUD 퀘스트 트래커 — 게임 화면 한쪽에 "진행중" 퀘스트만 간략히 나열.
 * 정의/런타임 상태는 ULastFPSQuestSubsystem(단일 소스)에서 읽고, Status==InProgress 만 필터.
 * OnQuestStateChanged 구독으로 상태 변화 시 갱신.
 */
UCLASS()
class LASTFPS_API ULastFPSQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

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

	/** 진행중 퀘스트 목록을 재구성 */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void RebuildTracker();

	/** OnQuestStateChanged 콜백 → 트래커 갱신 */
	UFUNCTION()
	void HandleQuestStateChanged();
};
