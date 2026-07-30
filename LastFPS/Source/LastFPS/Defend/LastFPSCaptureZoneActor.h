#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSCaptureZoneActor.generated.h"

class UBoxComponent;

/**
 * 점령 목표의 위치 앵커다.
 *
 * 판정·진행·복제는 인카운터가 붙여 주는 ULastFPSTimedObjectiveComponent 가 소유하고,
 * 이 액터는 "여기가 점령 구역"이라는 볼륨만 제공한다. 밸런스 값(점령 시간 등)은
 * 목표 정의 에셋이 들고 있으므로 배치 인스턴스에서 조정할 것은 박스 크기뿐이다.
 *
 * 레벨 계약: 프로파일의 목표 마커 태그 + 인카운터 식별자 + 정의의 ObjectiveTag 를
 * Actor Tag 로 함께 달아야 인카운터가 이 액터를 자기 목표로 인식한다.
 */
UCLASS()
class LASTFPS_API ALastFPSCaptureZoneActor : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSCaptureZoneActor();

private:
	/** 점령 판정에 쓰이는 볼륨. 목표 컴포넌트가 오버랩을 조회한다. */
	UPROPERTY(VisibleAnywhere, Category="LastFPS|Capture")
	TObjectPtr<UBoxComponent> Volume;
};
