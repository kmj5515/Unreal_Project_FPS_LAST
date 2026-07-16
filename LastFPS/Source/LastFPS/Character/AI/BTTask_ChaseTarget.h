#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ChaseTarget.generated.h"

namespace EPathFollowingRequestResult
{
	enum Type : int;
}

/**
 * Blackboard 의 TargetActor 를 향해 이동하는 추격 BT Task.
 *  - AttackRange(AttributeSet)를 수용 반경으로 MoveToActor. 움직이는 타깃을 따라간다.
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

	/** 시야가 있을 때 AttackRange에 곱하는 이동 수용 반경 비율이다. */
	UPROPERTY(EditAnywhere, Category="Movement", meta=(ClampMin="0.0", ClampMax="1.0"))
	float AttackRangeAcceptanceScale = 0.9f;

	/** 시야가 막혔을 때 장애물을 우회하며 접근할 이동 수용 반경(cm)이다. */
	UPROPERTY(EditAnywhere, Category="Movement", meta=(ClampMin="0.0", Units="cm"))
	float NoLineOfSightAcceptanceRadius = 60.f;

	/** 도착 판정에 AI Pawn의 내비게이션 에이전트 반경을 포함할지 결정한다. */
	UPROPERTY(EditAnywhere, Category="Movement|Reach Test")
	bool bReachTestIncludesAgentRadius = false;

	/** 도착 판정에 타깃 Actor의 충돌 반경을 포함할지 결정한다. */
	UPROPERTY(EditAnywhere, Category="Movement|Reach Test")
	bool bReachTestIncludesGoalRadius = false;

private:
	/** 현재 Pawn 과 타깃 사이 거리, 그리고 AttackRange 를 반환. 타깃 없으면 false. */
	bool GetTargetDistance(UBehaviorTreeComponent& OwnerComp, float& OutDistance, float& OutAttackRange, class AActor*& OutTarget) const;

	/** 노드의 도착 판정 설정을 반영해 이동을 요청한다. */
	EPathFollowingRequestResult::Type RequestChaseMove(
		class AAIController& AIController,
		class AActor& Target,
		float AcceptanceRadius) const;
};
