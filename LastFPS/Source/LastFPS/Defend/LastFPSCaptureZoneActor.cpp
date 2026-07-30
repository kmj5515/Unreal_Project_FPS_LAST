#include "Defend/LastFPSCaptureZoneActor.h"

#include "Components/BoxComponent.h"

ALastFPSCaptureZoneActor::ALastFPSCaptureZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	SetRootComponent(Volume);

	// 배치 직후 보이고 겹치도록 기본 크기를 준다 — 실제 점령 범위는 인스턴스에서 조정한다.
	Volume->InitBoxExtent(FVector(200.f, 200.f, 100.f));
	Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
	Volume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Volume->SetGenerateOverlapEvents(true);
}
