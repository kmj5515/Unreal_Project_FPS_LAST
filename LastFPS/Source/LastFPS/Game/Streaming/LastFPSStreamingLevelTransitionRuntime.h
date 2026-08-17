#pragma once

#include "CoreMinimal.h"
#include "Game/Streaming/LastFPSStreamingLevelTransitionSettings.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "LastFPSStreamingLevelTransitionRuntime.generated.h"

class ALastFPSCharacterBase;
class ALastFPSEnemyCharacter;
class ACharacter;
class APawn;
class ATriggerBox;
class ULastFPSArrivalContainmentPresentationComponent;
class USceneComponent;
class ULevelStreamingDynamic;
class ULastFPSEnemyDefinition;
class UMaterialInterface;
class UStaticMesh;
struct FStreamableHandle;

/**
 * 하나의 스트리밍 전환 경로를 소유한다.
 * 서버는 진입 판정과 이동을 담당하고 각 피어는 동일한 이름으로 목적지 레벨을 미리 로드한다.
 */
UCLASS(NotBlueprintable, Transient)
class LASTFPS_API ALastFPSStreamingLevelTransitionRuntime : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSStreamingLevelTransitionRuntime();

	void ConfigureRoute(
		const FLastFPSStreamingLevelTransitionRoute& InRoute,
		ATriggerBox& InTriggerVolume);

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void HandleDestinationLevelLoaded();

	UFUNCTION()
	void HandleDestinationLevelShown();

	UFUNCTION()
	void OnRep_Route();

	UFUNCTION()
	void OnRep_TransitionRequested();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyTransitionArrived(
		FVector_NetQuantize ArrivalLocation,
		FRotator ArrivalRotation);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyDelayedEnemyEncounterProgress(
		int32 DefeatedEnemyCount,
		int32 TotalEnemyCount,
		bool bEncounterCleared);

	void BeginDestinationPreload();
	void BeginArrivalContainmentPreload();
	void HandleArrivalContainmentLoaded();
	void CancelArrivalContainmentPreload();
	void HideArrivalContainment();
	void BeginDelayedEnemyContentPreload();
	void HandleDelayedEnemyContentLoaded();

	/**
	 * 정의 에셋만 로드하면 PawnClass(TSoftClassPtr) 는 미로드로 남는다.
	 * 스폰 시점의 동기 로드 히치를 없애려고 정의 로드 직후 클래스까지 미리 받아 둔다.
	 */
	void BeginDelayedEnemySpawnDependencyPreload();
	void HandleDelayedEnemySpawnDependenciesLoaded();
	void BeginDelayedEnemyWeaponDependencyPreload();
	void CancelDelayedEnemyContentPreload();
	void RequestDestinationVisibility();
	bool ResolveDestinationTransform(FTransform& OutTransform) const;
	bool ResolveDelayedEnemySpawnTransform(FTransform& OutTransform) const;
	void UpdateEncounterObjectiveMarker();
	void UnregisterEncounterObjectiveMarker();
	void CompletePendingPawnTransition();

	/** 목적지에서 폰끼리 겹치지 않도록 자리를 찾아 이동시킨다. 실패하면 false. */
	bool TeleportPawnToDestination(
		APawn& Pawn,
		const FTransform& DestinationTransform) const;

	bool BeginArrivalHold(APawn& Pawn);
	void HandleArrivalHoldFinished();
	void ReleaseArrivalHold();
	void ScheduleDelayedEnemySpawn();
	void SpawnDelayedEnemy();
	void HandleDelayedEnemyDeath(ALastFPSCharacterBase* DeadCharacter);
	FString MakeStreamingInstanceName() const;

	UPROPERTY(ReplicatedUsing=OnRep_Route)
	FLastFPSStreamingLevelTransitionRoute Route;

	UPROPERTY(ReplicatedUsing=OnRep_TransitionRequested)
	bool bTransitionRequested = false;

	UPROPERTY(Transient)
	TObjectPtr<ATriggerBox> TriggerVolume;

	UPROPERTY(Transient)
	TObjectPtr<ULevelStreamingDynamic> DestinationStreamingLevel;

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSEnemyDefinition> LoadedDelayedEnemyDefinition;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> EncounterObjectiveMarker;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULastFPSArrivalContainmentPresentationComponent>
		ArrivalContainmentPresentation;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> LoadedArrivalContainmentMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedArrivalContainmentMaterial;

	/**
	 * 도착 이동 제한이 걸린 캐릭터 1명과 그 직전 이동 모드.
	 * 파티원마다 걷기/낙하 등 직전 상태가 다를 수 있어 해제할 값을 개별로 들고 있는다.
	 */
	struct FHeldCharacter
	{
		TWeakObjectPtr<ACharacter> Character;
		uint8 MovementMode = 0;
		uint8 CustomMovementMode = 0;
	};

	/**
	 * 이 전환으로 이동시킬 플레이어 폰 전체.
	 * 트리거를 밟은 1명만 옮기면 나머지 파티원은 닫힌 트리거 앞에 남아 목적지로 갈 수단이 없다.
	 */
	TArray<TWeakObjectPtr<APawn>> PendingPawns;
	TArray<FHeldCharacter> HeldCharacters;
	TWeakObjectPtr<ALastFPSEnemyCharacter> SpawnedDelayedEnemy;
	TSharedPtr<FStreamableHandle> ArrivalContainmentLoadHandle;
	TSharedPtr<FStreamableHandle> DelayedEnemyContentLoadHandle;
	TSharedPtr<FStreamableHandle> DelayedEnemySpawnDependencyLoadHandle;
	TSharedPtr<FStreamableHandle> DelayedEnemyWeaponDependencyLoadHandle;
	FTimerHandle DelayedEnemySpawnTimerHandle;
	FTimerHandle ArrivalHoldTimerHandle;
	FTimerHandle ArrivalContainmentTimerHandle;
	bool bDestinationLoaded = false;
	bool bDelayedEnemySpawnScheduled = false;
	bool bDelayedEnemySpawnDue = false;
	bool bEncounterMarkerRegistered = false;
};
