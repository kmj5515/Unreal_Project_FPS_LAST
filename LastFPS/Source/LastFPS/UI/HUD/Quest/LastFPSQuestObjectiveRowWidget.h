#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/LastFPSHUDStyle.h"
#include "LastFPSQuestObjectiveRowWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UWidget;
struct FLastFPSTrackedObjective;

/**
 * 목표 줄 1개의 부가 표시 상태 — 거리 계산을 끝낸 카드가 만들어 넘긴다.
 * 목표 자체의 내용은 스냅샷이 담고 있어 여기에는 시청자에 따라 달라지는 값만 둔다.
 */
struct FLastFPSQuestObjectiveRowDisplay
{
	/** 안내 지점까지 남은 거리(m). bHasDistance 가 false 면 값이 없다. */
	float DistanceMeters = 0.f;

	bool bHasDistance = false;
};

/**
 * HUD 퀘스트 트래커의 목표 줄 1개 — 문구 + 진행 카운트/게이지 + 거리 + 완료 체크.
 * 요구량이 1인 목표는 "1/1" 카운트가 정보가 아니라 소음이라 카운트/게이지를 숨긴다.
 * WBP 계약(모두 선택): Text_Label / Text_Progress / Bar_Progress / Text_Distance / Icon_Pending / Icon_Complete.
 */
UCLASS()
class LASTFPS_API ULastFPSQuestObjectiveRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 카드가 갱신마다 호출 — 표시만 반영하고 판정은 하지 않는다. */
	void UpdateObjective(
		const FLastFPSTrackedObjective& Objective,
		const FLastFPSQuestObjectiveRowDisplay& Display);

protected:
	/** BP 확장 훅 — 완료 연출·강조 애니메이션용. */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Quest")
	void OnObjectiveRowUpdated(bool bCompleted, float ProgressRatio);

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Label;

	/** 진행 카운트 ("3 / 5"). 요구량 1이면 숨긴다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Progress;

	/** 진행 게이지. 요구량 1이면 숨긴다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Progress;

	/** 안내 지점까지의 거리 ("142m"). 위치를 해석할 수 없는 목표에서는 숨긴다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Distance;

	/** 미완료 목표 표식(◈ 등). 완료 시 숨긴다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Icon_Pending;

	/** 완료 목표 표식(✔ 등). 미완료 시 숨긴다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Icon_Complete;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Style")
	FLinearColor ActiveColor = LastFPSHUDStyle::QuestObjectiveActive();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Style")
	FLinearColor CompletedColor = LastFPSHUDStyle::QuestObjectiveDone();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Style")
	FLinearColor ProgressFillColor = LastFPSHUDStyle::QuestProgressFill();
};
