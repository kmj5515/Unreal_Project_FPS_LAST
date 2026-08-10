#pragma once

#include "UI/Framework/LastFPSContentScreenWidget.h"
#include "LastFPSQuestScreenWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class ULastFPSQuestEntryWidget;
class ULastFPSQuestDetailWidget;

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

	/**
	 * 화면이 활성화될 때마다 상세 패널을 닫아 둔다.
	 *
	 * WBP 기본값도 Collapsed 지만 그건 최초 생성 때만 적용된다. 이 화면은 레이어에서 pop 된 뒤에도
	 * 인스턴스가 살아남아 재활성화될 수 있어, 그때 직전 선택의 상세가 열린 채로 보인다.
	 * 진입 상태는 항상 "아무것도 선택되지 않음"이어야 하므로 활성화 시점에 명시적으로 되돌린다.
	 */
	virtual void NativeOnActivated() override;

	/** 행 하나를 그릴 위젯 클래스 (WBP_QuestEntry) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSubclassOf<ULastFPSQuestEntryWidget> EntryWidgetClass;

	/** 그룹 하나를 그릴 위젯 클래스 (WBP_QuestGroup) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSubclassOf<class ULastFPSQuestGroupWidget> GroupWidgetClass;

	/** 기존 단일 리스트 (호환성 유지용) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_QuestList;

	/** 메인 퀘스트 리스트 컨테이너 (아코디언용) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_MainQuests;

	/** 서브 퀘스트 리스트 컨테이너 (아코디언용) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_SideQuests;

	/** 퀘스트 상세 정보 위젯 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSQuestDetailWidget> DetailWidget;

	/** 행이 하나도 없을 때 보일 안내 텍스트 (선택) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Empty;

	/** 서브시스템 정의 테이블을 읽어 목록을 재구성 */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void RebuildQuestList();

	/** OnQuestStateChanged 콜백 → 목록 갱신 */
	UFUNCTION()
	void HandleQuestStateChanged();

	UFUNCTION()
	void HandleQuestSelected(FName QuestId);
};
