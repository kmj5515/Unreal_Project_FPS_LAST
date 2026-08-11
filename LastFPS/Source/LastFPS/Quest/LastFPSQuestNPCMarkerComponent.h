#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "Quest/LastFPSQuestMarkerTypes.h"
#include "LastFPSQuestNPCMarkerComponent.generated.h"

class ULastFPSQuestSubsystem;

/**
 * NPC 머리 위 퀘스트 마커.
 * 오너에서 받은 키에 대응하는 퀘스트가 있을 때만 위젯을 보여주고, 추적 중이면 강조 색으로 바뀐다.
 * 대화 목표 안내는 이 마커가 전담한다 — HUD 화면 마커는 대화 목표를 그리지 않는다.
 *
 * 표시 판정은 서브시스템의 GetNPCMarkerInfo 하나에만 의존한다 — 표시 규칙(순차 목표/맵 범위/
 * 체인 잠금)을 이 컴포넌트가 다시 구현하지 않기 위함이다. 위치는 부착으로 따라간다.
 * 위젯 클래스와 색·심볼 리소스는 프로젝트 설정/BP 에서 지정한다.
 */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSQuestNPCMarkerComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	ULastFPSQuestNPCMarkerComponent();

	/** 공통 NPC 설정에서 주입한 높이를 적용한다. 월드 시작 전후 어느 시점에도 안전하게 호출할 수 있다. */
	void SetMarkerHeight(float InMarkerHeight);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 머리 위로 띄울 높이(cm). 이 값이 마커 Z 위치의 단일 소스다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest", meta=(ClampMin="0"))
	float MarkerHeight = 100.f;

	/**
	 * 화면에 그릴 마커 한 변의 크기(픽셀). 스크린 스페이스라 거리와 무관하게 고정된다.
	 * 자동 크기(bDrawAtDesiredSize)는 등록 시점 크기가 0이라 쓸 수 없다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest", meta=(ClampMin="16"))
	float MarkerDrawSize = 128.f;

private:
	/** 퀘스트 상태 변경 통지 — 추적 대상이 바뀌었을 수 있으므로 표시를 다시 판정한다. */
	UFUNCTION()
	void HandleQuestStateChanged();

	/** 현재 추적 대상이 자기 키와 같을 때만 마커를 보인다. */
	void RefreshVisibility();

	/** 표시 판정에 쓸 키 — 오너가 응답하는 NPC 행 이름. */
	FName ResolveMarkerId() const;

	/** 이 마커가 가리키는 NPC 행 이름. None 이면 대응할 목표가 없다. */
	FName MarkerNPCRowName;

	/** 상태 변경마다 다시 해석하지 않도록 BeginPlay 에서 잡아 두는 서브시스템. */
	TWeakObjectPtr<ULastFPSQuestSubsystem> CachedQuestSubsystem;

	/** 마지막으로 위젯에 반영한 상태 — 같은 값이면 재적용을 건너뛴다. */
	FLastFPSQuestNPCMarkerInfo AppliedInfo;
	bool bHasAppliedInfo = false;
};
