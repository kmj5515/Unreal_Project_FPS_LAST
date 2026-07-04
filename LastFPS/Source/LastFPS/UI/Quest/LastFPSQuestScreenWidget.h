#pragma once

#include "UI/Framework/LastFPSContentScreenWidget.h"
#include "LastFPSQuestScreenWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class ULastFPSQuestEntryWidget;

/**
 * 임무(퀘스트) 목록 화면 — ContentScreen 크롬(타이틀/닫기) 위에 퀘스트 행을 나열.
 * 정의 테이블·런타임 상태·진행은 ULastFPSQuestSubsystem(단일 소스)에서 읽고,
 * OnQuestStateChanged 에 구독해 상태가 바뀌면 목록을 재구성한다.
 */
UCLASS()
class LASTFPS_API ULastFPSQuestScreenWidget : public ULastFPSContentScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 행 하나를 그릴 위젯 클래스 (WBP_QuestEntry) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSubclassOf<ULastFPSQuestEntryWidget> EntryWidgetClass;

	/** 엔트리를 담을 컨테이너 (VerticalBox / ScrollBox 등) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_QuestList;

	/** 행이 하나도 없을 때 보일 안내 텍스트 (선택) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Empty;

	/** 서브시스템 정의 테이블을 읽어 목록을 재구성 */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void RebuildQuestList();

	/** OnQuestStateChanged 콜백 → 목록 갱신 */
	UFUNCTION()
	void HandleQuestStateChanged();
};
