#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LastFPSStreamingLevelTransitionSubsystem.generated.h"

/** 현재 월드에 설정된 스트리밍 전환 경로를 런타임 액터로 구성한다. */
UCLASS()
class LASTFPS_API ULastFPSStreamingLevelTransitionSubsystem
	: public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};
