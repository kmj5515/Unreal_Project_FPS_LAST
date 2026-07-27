#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LastFPSRoomEncounterSubsystem.generated.h"

class ALastFPSRoomEncounterRuntime;
class UDataTable;
class ULastFPSDestinationContentComponent;
class ULastFPSRoomEncounterProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FLastFPSOnEncounterCleared,
	FName,
	EncounterId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FLastFPSOnEncounterProgressChanged,
	FName,
	EncounterId,
	int32,
	DefeatedEnemyCount,
	int32,
	TotalEnemyCount);

/**
 * 현재 GameMode의 Destination Content Set에 포함된 Room Encounter Profile을 소비해
 * 레벨 마커와 전투 데이터를 해석하고 런타임 Encounter를 생성한다.
 */
UCLASS()
class LASTFPS_API ULastFPSRoomEncounterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void NotifyEncounterCleared(
		FName EncounterId);

	void NotifyEncounterProgress(
		FName EncounterId,
		int32 DefeatedEnemyCount,
		int32 TotalEnemyCount);

	/** 현재 Mode의 중앙 Profile에서 이미 준비된 Encounter Table을 반환한다. */
	const UDataTable* GetEncounterTable() const;

#if !UE_BUILD_SHIPPING
	bool DebugClearEncounter(FName EncounterId);
#endif

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Encounter")
	FLastFPSOnEncounterCleared OnEncounterCleared;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Encounter")
	FLastFPSOnEncounterProgressChanged OnEncounterProgressChanged;

private:
	void HandleDestinationContentReady();

	void InitializeRuntimeEncounters(
		UWorld& InWorld,
		UDataTable& EncounterTable,
		const ULastFPSRoomEncounterProfile& Profile);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALastFPSRoomEncounterRuntime>> RuntimeEncounters;

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSRoomEncounterProfile> ActiveProfile;

	TWeakObjectPtr<ULastFPSDestinationContentComponent> DestinationContentComponent;
	FDelegateHandle ContentReadyHandle;
	bool bRuntimeEncountersInitialized = false;
};
