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
	void CancelDelayedEnemyContentPreload();
	void RequestDestinationVisibility();
	bool ResolveDestinationTransform(FTransform& OutTransform) const;
	bool ResolveDelayedEnemySpawnTransform(FTransform& OutTransform) const;
	void UpdateEncounterObjectiveMarker();
	void UnregisterEncounterObjectiveMarker();
	void CompletePendingPawnTransition();
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

	TWeakObjectPtr<APawn> PendingPawn;
	TWeakObjectPtr<ACharacter> HeldCharacter;
	TWeakObjectPtr<ALastFPSEnemyCharacter> SpawnedDelayedEnemy;
	TSharedPtr<FStreamableHandle> ArrivalContainmentLoadHandle;
	TSharedPtr<FStreamableHandle> DelayedEnemyContentLoadHandle;
	FTimerHandle DelayedEnemySpawnTimerHandle;
	FTimerHandle ArrivalHoldTimerHandle;
	FTimerHandle ArrivalContainmentTimerHandle;
	uint8 PreviousMovementMode = 0;
	uint8 PreviousCustomMovementMode = 0;
	bool bDestinationLoaded = false;
	bool bDelayedEnemySpawnScheduled = false;
	bool bDelayedEnemySpawnDue = false;
	bool bEncounterMarkerRegistered = false;
};
