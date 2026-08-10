#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSRoomEncounterProfile.h"
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
class ULastFPSTimedObjectiveComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
enum class ELastFPSObjectiveResult : uint8;
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
		const FLastFPSRoomEncounterData& InEncounterData,
		const ULastFPSRoomEncounterProfile& InProfile);

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
	void OnRep_BarrierPresentationSettings();

	UFUNCTION()
	void OnRep_EncounterProgress();

	UFUNCTION()
	void OnRep_EnemyDefinitionAssets();

	/** 목표가 성공·실패로 확정됐을 때 — 완료 재평가 또는 인카운터 실패 전파. */
	UFUNCTION()
	void HandleObjectiveResolved(UActorComponent* Objective, ELastFPSObjectiveResult ObjectiveResult);

	/**
	 * 행의 목표 정의와 레벨 배치물을 ObjectiveTag 로 짝지어 런타임 목표를 만든다.
	 * 짝을 찾지 못한 정의는 에러 로그를 남긴다(배치 누락이 조용히 섬멸형으로 흐르는 것을 막는다).
	 *
	 * 월드 초기화가 아니라 전투 시작 시점에 호출한다 — 목표 배치물이 스트리밍 서브레벨에 있으면
	 * 초기화 시점에는 아직 존재하지 않기 때문이다. 플레이어가 트리거를 밟은 시점에는 방이 로드돼 있다.
	 */
	void CreateObjectives();

	/** 모든 목표가 성공했는가. 목표가 없으면 참이다(기존 섬멸형과 동일). */
	bool AreAllObjectivesSucceeded() const;

	void StartObjectives();
	void StopObjectives();

	/** 게임플레이 실패 — 웨이브를 정지하고 배리어를 열며 실패를 전파한다. */
	void FailEncounter();

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

	/**
	 * 추적 목록에서 제거한다.
	 * bCountAsDefeated 가 false 면 처치 수를 올리지 않는다 — 목표 성공·실패로 남은 적을
	 * 강제 정리하는 경우가 처치로 집계되면 퀘스트 진행이 부풀기 때문이다.
	 */
	void RemoveTrackedEnemy(ALastFPSCharacterBase& Enemy, bool bCountAsDefeated = true);
	void RefreshRemainingEnemyMarkers();
	void BroadcastEncounterProgress() const;
	void EvaluateWaveCompletion();
	void CompleteEncounter();

	/** 구성 오류로 진행이 불가능할 때 — 플레이어가 갇히지 않도록 출구만 열고 끝낸다. */
	void AbortEncounterOnConfigurationError(const TCHAR* Reason);

	/** 남은 적을 즉시 정리한다(목표 성공·실패로 전투를 끝낼 때). */
	void ClearAliveEnemies();
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

	/** 클라이언트도 서버가 선택한 GameMode의 배리어 표현 계약을 사용한다. */
	UPROPERTY(ReplicatedUsing=OnRep_BarrierPresentationSettings)
	FLastFPSRoomBarrierPresentationSettings BarrierPresentationSettings;

	UPROPERTY(Replicated)
	bool bEncounterCleared = false;

	UPROPERTY(Replicated)
	int32 CurrentWave = 0;

	/**
	 * 웨이브 목록을 몇 번 되감았는가. 목표가 미해결이면 웨이브가 순환하므로 이 값이 오른다.
	 * HUD 표시용이자 추후 순환 난이도 스케일링의 입력이다.
	 */
	UPROPERTY(Replicated)
	int32 WaveLoopCount = 0;

	/** 이 방이 요구하는 목표들. 비어 있으면 섬멸형이다. 전투 시작 시 생성한다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSTimedObjectiveComponent>> Objectives;

	/** 행에서 받아 둔 목표 정의 목록. 실제 생성은 전투 시작까지 미룬다. */
	TArray<FLastFPSEncounterObjectiveEntry> ObjectiveEntries;

	/** 목표 배치물을 찾을 Actor Tag (프로파일의 레벨 계약). */
	FName ObjectiveMarkerTag;

	/** 목표를 이미 만들었는가 — 웨이브 순환으로 시작이 재진입해도 중복 생성하지 않는다. */
	bool bObjectivesCreated = false;

	/** 클라이언트 HUD도 같은 인카운터 진행 이벤트를 받도록 식별자와 카운트를 복제한다. */
	UPROPERTY(ReplicatedUsing=OnRep_EncounterProgress)
	FName EncounterId = NAME_None;

	/**
	 * 진행 표시의 분모. 웨이브가 순환하면 한 바퀴분을 더해 처치 수가 조기에 포화되지 않게 한다
	 * (분모가 고정이면 1회차 전멸 시점에 100%가 되어 퀘스트가 먼저 완료된다).
	 */
	UPROPERTY(ReplicatedUsing=OnRep_EncounterProgress)
	int32 TotalEnemyCount = 0;

	/** 웨이브 한 바퀴분 적 수. 순환 시 분모를 늘리는 단위다. */
	int32 BaseTotalEnemyCount = 0;

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
	int32 RemainingEnemyMarkerThreshold = 3;
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
