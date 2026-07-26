#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSCaptureZoneActor.generated.h"

class ULastFPSCaptureZoneComponent;

/**
 * 점령 목표를 레벨에 바로 배치하기 위한 액터. 루트가 점령 볼륨(ULastFPSCaptureZoneComponent)이라
 * 드롭 후 박스 크기와 ZoneTag/CaptureDuration 만 정하면 된다. 판정·퀘스트 통지는 컴포넌트가 소유하고
 * 이 액터는 배치 편의를 위한 껍데기다(방어의 DefendableDeviceActor 와 대칭).
 */
UCLASS()
class LASTFPS_API ALastFPSCaptureZoneActor : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSCaptureZoneActor();

private:
	UPROPERTY(VisibleAnywhere, Category="LastFPS|Capture")
	TObjectPtr<ULastFPSCaptureZoneComponent> CaptureZone;
};
