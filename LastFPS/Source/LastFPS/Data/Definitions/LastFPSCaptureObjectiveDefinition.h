#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSEncounterObjectiveDefinition.h"
#include "LastFPSCaptureObjectiveDefinition.generated.h"

/**
 * 점령 목표의 설정이다 — "구역 안에 머문 시간이 쌓이면 점령".
 *
 * 배치물의 볼륨을 진행 조건으로 연결하고 실패 감시 대상은 두지 않는다.
 * 방어와 다른 점은 이 배선 두 줄뿐이며, 진행·복제·판정은 같은 컴포넌트가 처리한다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSCaptureObjectiveDefinition : public ULastFPSEncounterObjectiveDefinition
{
	GENERATED_BODY()

public:
	ULastFPSCaptureObjectiveDefinition();

	/**
	 * 구역 안에 적이 있으면 점령 진행을 멈출지.
	 * 진행 "조건"을 강화할 뿐이라 별도 목표 유형이 필요하지 않다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Capture")
	bool bBlockedByEnemies = false;

protected:
	/** 배치물의 볼륨을 진행 조건으로 쓴다. 실패 감시 대상은 없다(점령은 실패하지 않는다). */
	virtual void ConfigureRuntimeObjective(AActor& Anchor, ULastFPSTimedObjectiveComponent& Objective) const override;
};
