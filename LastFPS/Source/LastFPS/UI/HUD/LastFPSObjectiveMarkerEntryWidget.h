#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSObjectiveMarkerEntryWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * 화면 마커 1개 — 목표 지점 아이콘 + 거리(m) + (화면 밖일 때) 방향 화살표.
 * 배치/투영은 컨테이너(ULastFPSObjectiveMarkerWidget)가 담당하고, 이 위젯은 표시만 갱신한다.
 */
UCLASS()
class LASTFPS_API ULastFPSObjectiveMarkerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 컨테이너가 매 프레임 호출 — 거리/화면밖 상태/방향각/라벨 반영. */
	void UpdateMarker(float DistanceMeters, bool bOffScreen, float ArrowAngleDeg, const FText& Label);

protected:
	/** 거리 텍스트 (예: "45m"). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Distance;

	/** 목표 라벨 (선택). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Label;

	/** 화면 밖일 때 회전시켜 방향을 가리키는 위젯(화살표 이미지 등). 화면 안이면 숨김. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Arrow;

	/** BP 확장 훅 — 마커 갱신 시 호출(추가 연출용). */
	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void OnMarkerUpdated(float DistanceMeters, bool bOffScreen, float ArrowAngleDeg);
};
