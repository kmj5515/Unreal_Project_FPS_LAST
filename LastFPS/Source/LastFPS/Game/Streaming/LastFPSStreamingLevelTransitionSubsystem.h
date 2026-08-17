#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LastFPSStreamingLevelTransitionSubsystem.generated.h"

class ULevel;
struct FLastFPSStreamingLevelTransitionRoute;

/** 현재 월드에 설정된 스트리밍 전환 경로를 런타임 액터로 구성한다. */
UCLASS()
class LASTFPS_API ULastFPSStreamingLevelTransitionSubsystem
	: public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	/** 트리거 볼륨을 찾아 런타임을 구성한다. 볼륨이 아직 없으면 false. */
	bool TryConfigureRoute(
		UWorld& InWorld,
		const FLastFPSStreamingLevelTransitionRoute& Route);

	void HandleLevelAddedToWorld(ULevel* InLevel, UWorld* InWorld);

	void StopWaitingForPendingRoutes();

	/**
	 * 트리거 볼륨이 스트리밍 서브레벨에 있으면 월드 BeginPlay 시점에는 존재하지 않는다.
	 * 그때 포기하면 그 경로는 영영 구성되지 않으므로, 서브레벨이 들어올 때마다 다시 시도한다.
	 * 요소는 프로젝트 설정(CDO)의 배열을 가리키며 그 수명은 프로세스 전체라 안전하다.
	 */
	TArray<const FLastFPSStreamingLevelTransitionRoute*> PendingRoutes;

	FDelegateHandle LevelAddedHandle;
};
