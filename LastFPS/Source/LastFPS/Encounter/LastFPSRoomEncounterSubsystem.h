#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LastFPSRoomEncounterSubsystem.generated.h"

class ALastFPSRoomEncounterRuntime;
class UDataTable;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLastFPSOnEncounterCleared, FName, EncounterId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FLastFPSOnEncounterProgressChanged,
	FName, EncounterId,
	int32, DefeatedEnemyCount,
	int32, TotalEnemyCount);

/** 레벨 마커를 데이터 계약으로 해석해 방별 런타임 전투를 생성한다. */
UCLASS()
class LASTFPS_API ULastFPSRoomEncounterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void NotifyEncounterCleared(FName EncounterId);
	void NotifyEncounterProgress(FName EncounterId, int32 DefeatedEnemyCount, int32 TotalEnemyCount);

#if !UE_BUILD_SHIPPING
	/** 서버에서 지정한 인카운터를 개발용 치트로 즉시 완료한다. */
	bool DebugClearEncounter(FName EncounterId);
#endif

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Encounter")
	FLastFPSOnEncounterCleared OnEncounterCleared;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Encounter")
	FLastFPSOnEncounterProgressChanged OnEncounterProgressChanged;

private:
	void HandleEncounterTableLoaded();
	void InitializeRuntimeEncounters(UWorld& InWorld, UDataTable& EncounterTable);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALastFPSRoomEncounterRuntime>> RuntimeEncounters;

	TSharedPtr<FStreamableHandle> EncounterTableLoadHandle;
};
