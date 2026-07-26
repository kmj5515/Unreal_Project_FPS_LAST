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
class ULastFPSRoomBarrierPresentationComponent;
class ULastFPSRoomSpawnPresentationComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
struct FStreamableHandle;

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
		const TArray<ATriggerBox*>& InBarrierVolumes,
		const TArray<ATargetPoint*>& InSpawnPoints,
		const FLastFPSRoomEncounterData& InEncounterData);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if !UE_BUILD_SHIPPING
	/** 시작된 인카운터에 한해 개발용 치트가 기존 클리어 경로를 안전하게 실행한다. */
	bool DebugForceCompleteEncounter();
	FName GetEncounterIdForDebug() const { return EncounterId; }
#endif

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
	void OnRep_BarrierVolumes();

	UFUNCTION()
	void OnRep_EncounterProgress();

	UFUNCTION()
	void OnRep_EnemyDefinitionAssets();

	void StartEncounter();
	void BeginEnemyDefinitionPreload();
	void HandleEnemyDefinitionPreloadCompleted();
	void CancelEnemyDefinitionPreload();
	bool ResolveLoadedEnemyDefinitions();
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
	void BroadcastEncounterProgress() const;
	void EvaluateWaveCompletion();
	void CompleteEncounter();
	void FailEncounterOpen(const TCHAR* Reason);
	void SetBarrierActive(bool bNewActive);
	void ConfigureBarrierPresentation();
	void BeginBarrierPresentationLoad();
	void HandleBarrierPresentationAssetsLoaded();
	void CancelBarrierPresentationLoad();
	void ResetBarrierPresentations();
	void ApplyBarrierState();
	FTransform ResolveSpawnTransform(int32 SpawnIndex);

	UPROPERTY(VisibleAnywhere, Category="Encounter")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(ReplicatedUsing=OnRep_BarrierVolumes)
	TArray<TObjectPtr<ATriggerBox>> BarrierVolumes;

	UPROPERTY(ReplicatedUsing=OnRep_BarrierState)
	bool bBarrierActive = false;

	UPROPERTY(Replicated)
	bool bEncounterCleared = false;

	UPROPERTY(Replicated)
	int32 CurrentWave = 0;

	/** 클라이언트 HUD도 같은 인카운터 진행 이벤트를 받도록 식별자와 카운트를 복제한다. */
	UPROPERTY(ReplicatedUsing=OnRep_EncounterProgress)
	FName EncounterId = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_EncounterProgress)
	int32 TotalEnemyCount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_EncounterProgress)
	int32 DefeatedEnemyCount = 0;

	/** 클라이언트도 첫 적 복제 전에 같은 Pawn 클래스를 비동기로 준비하도록 경로를 복제한다. */
	UPROPERTY(ReplicatedUsing=OnRep_EnemyDefinitionAssets)
	TArray<TSoftObjectPtr<ULastFPSCharacterDefinition>> EnemyDefinitionAssets;

	UPROPERTY(Transient)
	TObjectPtr<ATriggerBox> TriggerVolume;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATargetPoint>> SpawnPoints;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSCharacterDefinition>> LoadedEnemyDefinitions;

	UPROPERTY(VisibleAnywhere, Category="Encounter")
	TObjectPtr<ULastFPSRoomSpawnPresentationComponent> SpawnPresentationComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSRoomBarrierPresentationComponent>> BarrierPresentationComponents;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> LoadedBarrierMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedBarrierMaterial;

	TArray<FLastFPSRoomEncounterWaveDefinition> Waves;
	TSet<TWeakObjectPtr<ALastFPSCharacterBase>> AliveEnemies;
	TArray<int32> ActiveSpawnPointOrder;
	TArray<FLastFPSPendingRoomEnemySpawn> PendingEnemySpawns;
	FTimerHandle NextWaveTimerHandle;
	FTimerHandle SequentialSpawnTimerHandle;
	FTimerHandle SpawnPresentationDelayTimerHandle;
	float ReusedSpawnPointSpacing = 180.f;
	float SpawnPointRandomRadius = 600.f;
	float SpawnDelayAfterVFX = 0.f;
	int32 MaxSpawnedActorsPerFrame = 1;
	int32 ActiveWaveIndex = INDEX_NONE;
	int32 NextSpawnUnitIndex = 0;
	int32 SpawnedCountInCurrentUnit = 0;
	int32 CurrentWaveSpawnPointIndex = 0;
	int32 CurrentWaveSpawnedCount = 0;
	bool bInitialized = false;
	bool bEnemyDefinitionsReady = false;
	bool bEnemyDefinitionsFailed = false;
	bool bStartRequested = false;
	bool bEncounterStarted = false;
	bool bWaveSpawning = false;
	bool bLoggedSpawnProjectionFailure = false;
	TSharedPtr<FStreamableHandle> EnemyDefinitionLoadHandle;
	TSharedPtr<FStreamableHandle> BarrierPresentationLoadHandle;
};
