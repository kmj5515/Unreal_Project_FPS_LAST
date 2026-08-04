#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSObjectiveMarkerWidget.generated.h"

class UCanvasPanel;
class ULastFPSQuestSubsystem;
class ULastFPSObjectiveMarkerEntryWidget;
struct FLastFPSObjectiveWaypoint;

struct FLastFPSGroundedMarkerCacheEntry
{
	FVector SourceLocation = FVector::ZeroVector;
	FVector GroundLocation = FVector::ZeroVector;
	double NextTraceTime = 0.0;
	bool bHasGroundHit = false;
};

/**
 * HUD 목표 마커 컨테이너 — 진행중 퀘스트의 위치 목표(ReachLocation)를 화면에 3D 마커로 표시.
 * 매 프레임 서브시스템에서 웨이포인트를 받아 월드→화면 투영, 화면 밖이면 가장자리로 클램프(방향 화살표),
 * 아이콘 옆에 거리(m)를 표시한다. 마커 하나당 엔트리 위젯을 풀링해 재사용한다.
 * WBP 계약: 캔버스 패널 이름 Canvas_Markers, EntryWidgetClass 지정.
 */
UCLASS()
class LASTFPS_API ULastFPSObjectiveMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 마커 엔트리를 담는 캔버스. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UCanvasPanel> Canvas_Markers;

	/** 마커 하나를 그릴 엔트리 위젯 클래스 (WBP_ObjectiveMarkerEntry). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSubclassOf<ULastFPSObjectiveMarkerEntryWidget> EntryWidgetClass;

	/** 화면 밖 클램프 시 가장자리에서 띄울 여백(픽셀). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin="0"))
	float EdgeMargin = 48.f;

	/** 위치 목표 중심보다 위에서 바닥 탐색을 시작하는 높이(cm)이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Ground", meta=(ClampMin="0"))
	float GroundTraceHeight = 100.f;

	/** 위치 목표 아래로 바닥을 탐색하는 거리(cm)이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Ground", meta=(ClampMin="0"))
	float GroundTraceDepth = 2000.f;

	/** 바닥과 아이콘이 겹치지 않도록 적용하는 높이 오프셋(cm)이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Ground")
	float GroundMarkerOffset = 8.f;

	/** 바닥을 찾지 못했을 때 재탐색하는 주기(초)이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Ground", meta=(ClampMin="0.1"))
	float GroundTraceRetryInterval = 1.f;

	/** 바닥 판정에 사용할 충돌 채널이다. 기본값은 Visibility이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

private:
	ULastFPSQuestSubsystem* GetQuestSubsystem() const;

	/** 현재 웨이포인트로 마커 위치/거리/화살표를 갱신. */
	void RefreshMarkers();

	/** 위치 목표의 트리거 중심을 실제 바닥 좌표로 투영하고 결과를 캐시한다. */
	FVector ResolveGroundLocation(
		const FLastFPSObjectiveWaypoint& Waypoint,
		class APlayerController* PlayerController);

	/** 더 이상 활성 상태가 아닌 위치 목표의 바닥 캐시를 제거한다. */
	void PruneGroundLocationCache(const TArray<FLastFPSObjectiveWaypoint>& Waypoints);

	/** 월드 좌표를 화면(픽셀)로 투영 — 화면 밖이면 가장자리로 클램프하고 방향각을 낸다. */
	static void ComputeScreenPosition(
		class APlayerController* PC,
		const FVector& WorldLocation,
		const FVector2D& ViewportSize,
		float Margin,
		FVector2D& OutScreenPos,
		bool& bOutOffScreen,
		float& OutAngleDeg);

	/** 재사용 엔트리 풀 (웨이포인트 수에 맞춰 늘어남). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSObjectiveMarkerEntryWidget>> MarkerEntries;

	TMap<FName, FLastFPSGroundedMarkerCacheEntry> GroundLocationCache;
};
