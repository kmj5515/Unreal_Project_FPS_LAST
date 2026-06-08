#pragma once

#include "UI/LastFPSContentScreenWidget.h"
#include "LastFPSQuestScreenWidget.generated.h"

class UDataTable;
class UPanelWidget;
class UTextBlock;
class ULastFPSQuestEntryWidget;

/**
 * 임무(퀘스트) 목록 화면 — ContentScreen 크롬(타이틀/닫기) 위에 퀘스트 행을 나열.
 * QuestTable의 모든 행을 EntryWidgetClass 인스턴스로 만들어 Box_QuestList에 채운다.
 * 진행 추적 서브시스템은 아직 없음 — 행의 Status 값을 그대로 표시(프로토).
 */
UCLASS()
class LASTFPS_API ULastFPSQuestScreenWidget : public ULastFPSContentScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	/** 퀘스트 정의 테이블 (RowType = FLastFPSQuestData) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(RequiredAssetDataTags="RowStructure=/Script/LastFPS.LastFPSQuestData"))
	TObjectPtr<UDataTable> QuestTable;

	/** 행 하나를 그릴 위젯 클래스 (WBP_QuestEntry) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSubclassOf<ULastFPSQuestEntryWidget> EntryWidgetClass;

	/** 엔트리를 담을 컨테이너 (VerticalBox / ScrollBox 등) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_QuestList;

	/** 행이 하나도 없을 때 보일 안내 텍스트 (선택) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Empty;

	/** 테이블을 다시 읽어 목록을 재구성 */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void RebuildQuestList();
};
