#include "Character/AI/BTTask_ChaseTarget.h"

#include "Character/AI/LastFPSEnemyBlackboardKeys.h"
#include "Character/LastFPSEnemyCharacter.h"

#include "AIController.h"
#include "AITypes.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_ChaseTarget::UBTTask_ChaseTarget()
{
	NodeName = TEXT("Chase Target");
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ChaseTarget, TargetActorKey), AActor::StaticClass());
}

void UBTTask_ChaseTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBData = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBData);
	}
}

bool UBTTask_ChaseTarget::GetTargetDistance(UBehaviorTreeComponent& OwnerComp, float& OutDistance, float& OutAttackRange, AActor*& OutTarget) const
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	ALastFPSEnemyCharacter* Enemy = AICon ? Cast<ALastFPSEnemyCharacter>(AICon->GetPawn()) : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Enemy || !BB)
	{
		return false;
	}

	OutTarget = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!OutTarget)
	{
		return false;
	}

	// 공격 사거리는 GE로 변경될 수 있는 전투 스탯이므로 AttributeSet만 사용한다.
	OutAttackRange = Enemy->GetAttackRange();
	OutDistance = FVector::Dist(Enemy->GetActorLocation(), OutTarget->GetActorLocation());
	return true;
}

EPathFollowingRequestResult::Type UBTTask_ChaseTarget::RequestChaseMove(
	AAIController& AIController,
	AActor& Target,
	const float AcceptanceRadius) const
{
	FAIMoveRequest MoveRequest(&Target);
	MoveRequest.SetAcceptanceRadius(FMath::Max(AcceptanceRadius, 0.f));
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAllowPartialPath(true);
	MoveRequest.SetCanStrafe(true);
	MoveRequest.SetNavigationFilter(AIController.GetDefaultNavigationFilterClass());
	MoveRequest.SetReachTestIncludesAgentRadius(bReachTestIncludesAgentRadius);
	MoveRequest.SetReachTestIncludesGoalRadius(bReachTestIncludesGoalRadius);

	if (AIController.GetMoveStatus() != EPathFollowingStatus::Idle)
	{
		AIController.StopMovement();
	}

	return AIController.MoveTo(MoveRequest).Code;
}

EBTNodeResult::Type UBTTask_ChaseTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AICon = OwnerComp.GetAIOwner();

	float Distance = 0.f, AttackRange = 0.f;
	AActor* Target = nullptr;
	if (!AICon || !GetTargetDistance(OwnerComp, Distance, AttackRange, Target))
	{
		return EBTNodeResult::Failed;
	}

	AICon->SetFocus(Target);

	const bool bHasLoS = AICon->LineOfSightTo(Target);

	// 사거리 안 + 시야 확보 시에만 멈춘다. 시야가 막혔으면 더 접근해 시야를 확보한다.
	if (Distance <= AttackRange && bHasLoS)
	{
		return EBTNodeResult::Succeeded;
	}

	// 시야가 있으면 사거리 살짝 안쪽까지, 없으면 더 바짝 붙어 장애물을 우회한다.
	const float Acceptance = bHasLoS
		? AttackRange * AttackRangeAcceptanceScale
		: NoLineOfSightAcceptanceRadius;
	const EPathFollowingRequestResult::Type MoveResult = RequestChaseMove(*AICon, *Target, Acceptance);
	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		AICon->ClearFocus(EAIFocusPriority::Gameplay);
		return EBTNodeResult::Failed;
	}
	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::InProgress;
}

void UBTTask_ChaseTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float /*DeltaSeconds*/)
{
	AAIController* AICon = OwnerComp.GetAIOwner();

	float Distance = 0.f, AttackRange = 0.f;
	AActor* Target = nullptr;
	if (!AICon || !GetTargetDistance(OwnerComp, Distance, AttackRange, Target))
	{
		// 타깃 소실 → 이동 중단 후 실패로 종료(상위 트리가 재판단).
		if (AICon)
		{
			AICon->StopMovement();
			AICon->ClearFocus(EAIFocusPriority::Gameplay);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AICon->SetFocus(Target);

	const bool bHasLoS = AICon->LineOfSightTo(Target);

	if (Distance <= AttackRange && bHasLoS)
	{
		// 사거리 진입 + 시야 확보 → 공격 분기로 넘김.
		AICon->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 정지 상태로 남아 있으면(경로 종료/실패) 다시 이동 지시. 움직이는 타깃을 계속 따라간다.
	// 시야가 막혔으면 사거리 안이어도 더 접근해 시야를 확보한다.
	if (AICon->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		const float Acceptance = bHasLoS
			? AttackRange * AttackRangeAcceptanceScale
			: NoLineOfSightAcceptanceRadius;
		const EPathFollowingRequestResult::Type MoveResult = RequestChaseMove(*AICon, *Target, Acceptance);
		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			AICon->ClearFocus(EAIFocusPriority::Gameplay);
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
		else if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}
