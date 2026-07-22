#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "LastFPSRoomEncounterSettings.generated.h"

class UDataTable;

/**
 * 방 전투의 공통 밸런스와 레벨 마커 계약을 제공한다.
 * 레벨은 Trigger/Barrier/Spawn 마커와 동일한 방 식별자 태그만 배치하면 된다.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Room Encounter Settings"))
class LASTFPS_API ULastFPSRoomEncounterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Encounter 식별 태그와 Row Name을 연결하는 전투 구성 테이블이다. */
	UPROPERTY(Config, EditAnywhere, Category="Data", meta=(RowType="/Script/LastFPS.LastFPSRoomEncounterData"))
	TSoftObjectPtr<UDataTable> EncounterTable;

	UPROPERTY(Config, EditAnywhere, Category="Level Contract")
	FName TriggerMarkerTag = TEXT("RoomEncounter.Trigger");

	UPROPERTY(Config, EditAnywhere, Category="Level Contract")
	FName BarrierMarkerTag = TEXT("RoomEncounter.Barrier");

	UPROPERTY(Config, EditAnywhere, Category="Level Contract")
	FName SpawnMarkerTag = TEXT("RoomEncounter.Spawn");
};
