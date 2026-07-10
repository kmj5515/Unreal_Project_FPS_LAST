#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ChaseTarget.generated.h"

/**
 * Blackboard 의 TargetActor 를 향해 이동하는 추격 BT Task.
 *  - AttackRange(AIProfile) 를 수용 반경으로 MoveToActor. 움직이는 타깃을 따라간다.
 *  - 사거리 안에 들어오면 Succeeded 로 종료 → 상위 트리가 공격 분기로 전환.
 *  - 타깃이 사라지면 Failed.
 * 이동 중 타깃을 SetFocus 로 바라보게 해 몸이 타깃을 향하도록 한다.
 */
UCLASS()
class LASTFPS_API UBTTask_ChaseTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ChaseTarget();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	/** 추격 대상 Actor 키. */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetActorKey;

private:
	/** 현재 Pawn 과 타깃 사이 거리, 그리고 AttackRange 를 반환. 타깃 없으면 false. */
	bool GetTargetDistance(UBehaviorTreeComponent& OwnerComp, float& OutDistance, float& OutAttackRange, class AActor*& OutTarget) const;
};
