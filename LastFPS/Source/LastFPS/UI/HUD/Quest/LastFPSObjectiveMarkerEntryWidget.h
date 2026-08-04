#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSObjectiveMarkerEntryWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * 마커 1개의 표시 상태 — 배치/투영을 끝낸 컨테이너가 계산해 엔트리에 넘긴다.
 * 표시 항목이 늘어도 엔트리 호출 규약이 흔들리지 않도록 인자를 묶어 전달한다.
 */
struct FLastFPSObjectiveMarkerDisplay
{
	/** 목표까지 남은 거리(m). 동선 지점에는 표시하지 않는다. */
	float DistanceMeters = 0.f;

	/** 화면 밖일 때 가장자리에서 가리킬 방향(도). */
	float ArrowAngleDeg = 0.f;

	/** 화면 밖으로 클램프된 상태인가. */
	bool bOffScreen = false;

	/** 목적지가 아니라 이동 동선을 안내하는 중간 지점인가. */
	bool bIsRoutePoint = false;

	/** 목표 라벨. 동선 지점은 비운다. */
	FText Label;
};

/**
 * 화면 마커 1개 — 목표 지점 아이콘 + 거리(m) + (화면 밖일 때) 방향 화살표.
 * 배치/투영은 컨테이너(ULastFPSObjectiveMarkerWidget)가 담당하고, 이 위젯은 표시만 갱신한다.
 * 이동 동선 지점은 거리/라벨 없이 경로 아이콘만 보여 목적지 마커와 구분한다.
 */
UCLASS()
class LASTFPS_API ULastFPSObjectiveMarkerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 컨테이너가 매 프레임 호출 — 표시 상태를 반영한다. */
	void UpdateMarker(const FLastFPSObjectiveMarkerDisplay& Display);

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

	/** 목적지 아이콘 (선택) — 동선 지점일 때 숨긴다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Icon_Destination;

	/** 이동 동선 아이콘 (선택) — 목적지일 때 숨긴다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Icon_Route;

	/** BP 확장 훅 — 마커 갱신 시 호출(추가 연출용). */
	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void OnMarkerUpdated(float DistanceMeters, bool bOffScreen, float ArrowAngleDeg);
};
