#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LastFPSRoomEncounterSubsystem.generated.h"

class ALastFPSRoomEncounterRuntime;

/** 레벨 마커를 데이터 계약으로 해석해 방별 런타임 전투를 생성한다. */
UCLASS()
class LASTFPS_API ULastFPSRoomEncounterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ALastFPSRoomEncounterRuntime>> RuntimeEncounters;
};
