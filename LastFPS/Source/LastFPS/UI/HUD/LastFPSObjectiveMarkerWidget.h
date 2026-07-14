#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSObjectiveMarkerWidget.generated.h"

class UCanvasPanel;
class ULastFPSQuestSubsystem;
class ULastFPSObjectiveMarkerEntryWidget;

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

private:
	ULastFPSQuestSubsystem* GetQuestSubsystem() const;

	/** 현재 웨이포인트로 마커 위치/거리/화살표를 갱신. */
	void RefreshMarkers();

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
};
