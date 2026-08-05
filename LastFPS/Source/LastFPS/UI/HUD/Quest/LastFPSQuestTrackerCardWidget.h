#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "UI/HUD/LastFPSHUDStyle.h"
#include "LastFPSQuestTrackerCardWidget.generated.h"

class UImage;
class UPanelWidget;
class UTextBlock;
class ULastFPSQuestObjectiveRowWidget;
struct FLastFPSTrackedQuest;

/**
 * 거리 계산의 기준이 되는 시청자 위치 — 트래커가 갱신마다 1회 구해 모든 카드에 넘긴다.
 * 폰/카메라를 아직 못 잡은 프레임에는 bValid 가 false 라 거리를 표시하지 않는다.
 */
struct FLastFPSQuestTrackerViewer
{
	FVector Location = FVector::ZeroVector;

	bool bValid = false;
};

/**
 * HUD 퀘스트 트래커의 퀘스트 카드 1개 — 분류(메인/서브) + 제목 + 목표 줄 목록.
 * 목표 줄 위젯은 풀링해 재사용하고, 남는 줄은 파괴하지 않고 접어 갱신 시 할당이 생기지 않게 한다.
 * WBP 계약: Box_Objectives(필수) / Text_Category · Text_Title · Img_Accent(선택), ObjectiveRowClass 지정.
 */
UCLASS()
class LASTFPS_API ULastFPSQuestTrackerCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 트래커가 갱신마다 호출 — 스냅샷을 그대로 그리고 거리만 시청자 위치로 계산한다. */
	void UpdateQuest(const FLastFPSTrackedQuest& Quest, const FLastFPSQuestTrackerViewer& Viewer);

protected:
	/** BP 확장 훅 — 카드가 다른 퀘스트로 바뀌었을 때(bQuestChanged) 등장 연출을 걸 수 있다. */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Quest")
	void OnQuestCardUpdated(ELastFPSQuestType QuestType, bool bQuestChanged);

	/** 목표 줄을 담을 컨테이너 (VerticalBox 등). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPanelWidget> Box_Objectives;

	/** 분류 표시 ("메인" / "서브"). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Category;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	/** 좌측 세로 강조 바 — 분류별 색으로 메인/서브를 한눈에 구분한다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_Accent;

	/** 목표 줄 하나를 그릴 위젯 클래스 (WBP_QuestObjectiveRow). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest")
	TSubclassOf<ULastFPSQuestObjectiveRowWidget> ObjectiveRowClass;

	/** 카드 하나에 표시할 최대 목표 줄 수. 순차 퀘스트는 완료 단계가 쌓이므로 가장 긴 퀘스트의 단계 수를 담아야 한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest", meta=(ClampMin="1"))
	int32 MaxObjectiveRows = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Style")
	FLinearColor MainAccentColor = LastFPSHUDStyle::QuestMainAccent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Style")
	FLinearColor SideAccentColor = LastFPSHUDStyle::QuestSideAccent();

private:
	/** 필요한 만큼 목표 줄을 확보한다 (부족한 만큼만 생성). */
	ULastFPSQuestObjectiveRowWidget* AcquireObjectiveRow(int32 RowIndex);

	/** 재사용 줄 풀 — 목표 수에 맞춰 늘어나고, 남는 줄은 접어 둔다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSQuestObjectiveRowWidget>> ObjectiveRows;

	FName DisplayedQuestId;

	bool bObjectiveRowClassWarningLogged = false;
};
