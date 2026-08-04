#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "LastFPSQuestTrackerWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class ULastFPSQuestTrackerCardWidget;
struct FLastFPSQuestTrackerViewer;

/**
 * HUD 퀘스트 트래커 — 진행중 퀘스트를 카드로 나열하고, 카드마다 목표 줄(카운트/게이지/거리)을 보인다.
 * 정의/런타임 상태는 ULastFPSQuestSubsystem 의 표시 스냅샷(GetTrackedQuests)만 소비하므로
 * 목표 판정 규칙을 UI 가 다시 구현하지 않는다. 메인 퀘스트를 먼저 두고 나머지는 테이블 행 순서를 지킨다.
 *
 * 갱신 경로가 둘이다: 상태 변경 브로드캐스트(즉시 반영)와 거리 갱신 타이머(안내 지점이 있을 때만 가동).
 * WBP 계약: Box_TrackerList(카드 컨테이너) / TB_Empty(선택), QuestCardWidgetClass 지정.
 *
 * 전투 HUD 와 동시에 상시 노출돼야 하므로 CommonUI 레이어 스택(맨 위 1개만 표시)이 아니라
 * 목표 마커 오버레이와 같이 뷰포트에 직접 얹는다. 그래서 활성화 개념이 필요 없는 UUserWidget 이다.
 */
UCLASS()
class LASTFPS_API ULastFPSQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 퀘스트 카드 하나를 그릴 위젯 클래스 (WBP_QuestTrackerCard). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSubclassOf<ULastFPSQuestTrackerCardWidget> QuestCardWidgetClass;

	/** 표시할 최대 진행중 퀘스트 수. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin="1"))
	int32 MaxTrackedQuests = 3;

	/**
	 * 거리 표시 갱신 주기(초). 거리는 플레이어가 움직이면 계속 바뀌지만 텍스트는 1m 단위라
	 * 매 프레임 Tick 대신 타이머로 갱신하고, 안내 지점이 있는 목표가 없으면 타이머를 끈다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin="0.05", Units="s"))
	float DistanceRefreshInterval = 0.2f;

	/** 카드를 담을 컨테이너 (VerticalBox 등). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_TrackerList;

	/** 진행중 퀘스트가 하나도 없을 때 보일 안내 텍스트 (선택). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Empty;

	/** 진행중 퀘스트 목록과 목표 진행/거리를 다시 반영한다. */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void RefreshTracker();

	/** OnQuestStateChanged 콜백 → 트래커 갱신. */
	UFUNCTION()
	void HandleQuestStateChanged();

private:
	/** 필요한 만큼 카드를 확보한다 (부족한 만큼만 생성). */
	ULastFPSQuestTrackerCardWidget* AcquireQuestCard(int32 CardIndex);

	/** 거리 기준 위치 — 플레이어 폰이 없으면 카메라 시점으로 대체한다. */
	FLastFPSQuestTrackerViewer ResolveViewer() const;

	/** 안내 지점이 있는 목표 유무에 따라 거리 갱신 타이머를 켜고/끈다. */
	void UpdateDistanceRefreshTimer(bool bNeedsDistanceRefresh);

	ULastFPSQuestSubsystem* GetQuestSubsystem() const;

	/** 재사용 카드 풀 — 표시 퀘스트 수에 맞춰 늘어나고, 남는 카드는 접어 둔다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSQuestTrackerCardWidget>> QuestCards;

	/** 갱신마다 재할당되지 않도록 유지하는 스냅샷 버퍼. */
	TArray<FLastFPSTrackedQuest> TrackedQuests;

	FTimerHandle DistanceRefreshTimerHandle;

	bool bQuestCardClassWarningLogged = false;
};
