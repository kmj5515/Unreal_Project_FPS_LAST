#pragma once

#include "CoreMinimal.h"
#include "Data/Tables/LastFPSRoomEncounterData.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPtr.h"
#include "LastFPSRoomEncounterRuntime.generated.h"

class ALastFPSCharacterBase;
class ATargetPoint;
class ATriggerBox;
class ULastFPSCharacterDefinition;
class ULastFPSRoomSpawnPresentationComponent;

/** 이펙트 예고 후 실제 생성을 기다리는 적 한 마리의 불변 요청이다. */
struct FLastFPSPendingRoomEnemySpawn
{
	TObjectPtr<ULastFPSCharacterDefinition> Definition;
	FTransform SpawnTransform = FTransform::Identity;
	int32 SpawnIndex = INDEX_NONE;
};

/** 서버에서 한 방의 웨이브 진행과 출구 차단을 소유하는 런타임 액터다. */
UCLASS(NotBlueprintable, Transient)
class LASTFPS_API ALastFPSRoomEncounterRuntime : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSRoomEncounterRuntime();

	void InitializeEncounter(
		FName InEncounterId,
		ATriggerBox& InTriggerVolume,
		ATriggerBox& InBarrierVolume,
		const TArray<ATargetPoint*>& InSpawnPoints,
		const FLastFPSRoomEncounterData& InEncounterData);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnRep_BarrierState();

	UFUNCTION()
	void OnRep_BarrierVolume();

	void StartEncounter();
	bool LoadEnemyDefinitions();
	void ScheduleNextWave();
	void SpawnNextWave();
	void SpawnNextQueuedEnemy();
	bool QueueOneEnemySpawn();
	void CommitPendingEnemySpawns();
	void ScheduleNextSpawnBatch();
	bool NormalizeSpawnCursor();
	void PrepareSpawnPointOrder(const FLastFPSRoomEncounterWaveDefinition& Wave);
	void FinishWaveSpawning();
	ALastFPSCharacterBase* SpawnEnemy(
		ULastFPSCharacterDefinition& Definition,
		const FTransform& SpawnTransform,
		int32 SpawnIndex);
	void HandleEnemyDeath(ALastFPSCharacterBase* DeadCharacter);
	void RemoveTrackedEnemy(ALastFPSCharacterBase& Enemy);
	void EvaluateWaveCompletion();
	void CompleteEncounter();
	void FailEncounterOpen(const TCHAR* Reason);
	void SetBarrierActive(bool bNewActive);
	void ApplyBarrierCollision() const;
	FTransform ResolveSpawnTransform(int32 SpawnIndex);

	UPROPERTY(ReplicatedUsing=OnRep_BarrierVolume)
	TObjectPtr<ATriggerBox> BarrierVolume;

	UPROPERTY(ReplicatedUsing=OnRep_BarrierState)
	bool bBarrierActive = false;

	UPROPERTY(Replicated)
	bool bEncounterCleared = false;

	UPROPERTY(Replicated)
	int32 CurrentWave = 0;

	UPROPERTY(Transient)
	TObjectPtr<ATriggerBox> TriggerVolume;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATargetPoint>> SpawnPoints;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSCharacterDefinition>> LoadedEnemyDefinitions;

	UPROPERTY(VisibleAnywhere, Category="Encounter")
	TObjectPtr<ULastFPSRoomSpawnPresentationComponent> SpawnPresentationComponent;

	TArray<FLastFPSRoomEncounterWaveDefinition> Waves;
	TSet<TWeakObjectPtr<ALastFPSCharacterBase>> AliveEnemies;
	TArray<int32> ActiveSpawnPointOrder;
	TArray<FLastFPSPendingRoomEnemySpawn> PendingEnemySpawns;
	FTimerHandle NextWaveTimerHandle;
	FTimerHandle SequentialSpawnTimerHandle;
	FTimerHandle SpawnPresentationDelayTimerHandle;
	FName EncounterId = NAME_None;
	float ReusedSpawnPointSpacing = 180.f;
	float SpawnPointRandomRadius = 600.f;
	float SpawnDelayAfterVFX = 0.f;
	int32 ActiveWaveIndex = INDEX_NONE;
	int32 NextSpawnUnitIndex = 0;
	int32 SpawnedCountInCurrentUnit = 0;
	int32 CurrentWaveSpawnPointIndex = 0;
	int32 CurrentWaveSpawnedCount = 0;
	bool bInitialized = false;
	bool bEncounterStarted = false;
	bool bWaveSpawning = false;
	bool bLoggedSpawnProjectionFailure = false;
};
