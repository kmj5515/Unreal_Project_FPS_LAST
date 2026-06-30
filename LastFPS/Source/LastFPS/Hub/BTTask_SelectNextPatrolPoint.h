#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SelectNextPatrolPoint.generated.h"

/**
 * 순찰 NPC의 다음 순찰 지점을 선택해 Blackboard에 기록하는 BT Task.
 * - Pawn(ALastFPSPatrolNPC)의 PatrolPoints / CurrentPatrolIndex 를 읽어
 *   다음 지점의 위치를 TargetLocationKey 에, 대기 시간을 WaitTimeKey 에 쓴다.
 * - 인덱스를 증가시켜 순환(loop)한다.
 * BT 배치: [SelectNextPatrolPoint] → [MoveTo: TargetLocation] → [Wait: WaitTime]
 */
UCLASS()
class LASTFPS_API UBTTask_SelectNextPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SelectNextPatrolPoint();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 다음 지점의 월드 위치를 기록할 Vector 키 (MoveTo가 사용) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetLocationKey;

	/** 지점 도착 후 대기 시간을 기록할 Float 키 (Wait가 사용) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector WaitTimeKey;
};
