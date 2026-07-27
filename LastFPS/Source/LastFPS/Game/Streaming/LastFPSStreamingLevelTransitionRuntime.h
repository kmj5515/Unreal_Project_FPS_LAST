#pragma once

#include "CoreMinimal.h"
#include "Game/Streaming/LastFPSStreamingLevelTransitionSettings.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "LastFPSStreamingLevelTransitionRuntime.generated.h"

class ALastFPSEnemyCharacter;
class APawn;
class ATriggerBox;
class ULevelStreamingDynamic;
class ULastFPSEnemyDefinition;
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

	void BeginDestinationPreload();
	void BeginDelayedEnemyContentPreload();
	void HandleDelayedEnemyContentLoaded();
	void CancelDelayedEnemyContentPreload();
	void RequestDestinationVisibility();
	bool ResolveDestinationTransform(FTransform& OutTransform) const;
	bool ResolveDelayedEnemySpawnTransform(FTransform& OutTransform) const;
	void CompletePendingPawnTransition();
	void ScheduleDelayedEnemySpawn();
	void SpawnDelayedEnemy();
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

	TWeakObjectPtr<APawn> PendingPawn;
	TWeakObjectPtr<ALastFPSEnemyCharacter> SpawnedDelayedEnemy;
	TSharedPtr<FStreamableHandle> DelayedEnemyContentLoadHandle;
	FTimerHandle DelayedEnemySpawnTimerHandle;
	bool bDestinationLoaded = false;
	bool bDelayedEnemySpawnScheduled = false;
	bool bDelayedEnemySpawnDue = false;
};
