#include "Character/AI/BTTask_ChaseTarget.h"

#include "Character/AI/LastFPSEnemyBlackboardKeys.h"
#include "Character/LastFPSAIProfile.h"
#include "Character/LastFPSEnemyCharacter.h"

#include "AIController.h"
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

	const ULastFPSAIProfile* Profile = Enemy->GetAIProfile();
	OutAttackRange = Profile ? Profile->AttackRange : 200.f;
	OutDistance = FVector::Dist(Enemy->GetActorLocation(), OutTarget->GetActorLocation());
	return true;
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

	// 이미 사거리 안이면 추격 불필요.
	if (Distance <= AttackRange)
	{
		AICon->SetFocus(Target);
		return EBTNodeResult::Succeeded;
	}

	// 타깃을 바라보며, 사거리 살짝 안쪽까지 이동(경계에서 진동 방지).
	AICon->SetFocus(Target);
	AICon->MoveToActor(Target, AttackRange * 0.9f, /*bStopOnOverlap*/ true, /*bUsePathfinding*/ true);
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

	if (Distance <= AttackRange)
	{
		// 사거리 진입 → 공격 분기로 넘김.
		AICon->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 정지 상태로 남아 있으면(경로 종료/실패) 다시 이동 지시. 움직이는 타깃을 계속 따라간다.
	if (AICon->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		AICon->MoveToActor(Target, AttackRange * 0.9f, true, true);
	}
}
