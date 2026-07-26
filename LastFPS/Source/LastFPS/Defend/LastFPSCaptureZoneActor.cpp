#include "Defend/LastFPSCaptureZoneActor.h"

#include "Defend/LastFPSCaptureZoneComponent.h"

ALastFPSCaptureZoneActor::ALastFPSCaptureZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	CaptureZone = CreateDefaultSubobject<ULastFPSCaptureZoneComponent>(TEXT("CaptureZone"));
	SetRootComponent(CaptureZone);

	// 배치 직후 보이고 겹치도록 기본 볼륨 크기 부여 — 실제 점령 범위는 인스턴스에서 조정.
	CaptureZone->InitBoxExtent(FVector(200.f, 200.f, 100.f));
}
