#include "Hub/BTTask_SelectNextPatrolPoint.h"

#include "Hub/LastFPSNPC.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SelectNextPatrolPoint::UBTTask_SelectNextPatrolPoint()
{
	NodeName = TEXT("Select Next Patrol Point");

	// 키 필터 — 에디터에서 Vector/Float 키만 선택되도록.
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SelectNextPatrolPoint, TargetLocationKey));
	WaitTimeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SelectNextPatrolPoint, WaitTimeKey));
}

void UBTTask_SelectNextPatrolPoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBData = GetBlackboardAsset())
	{
		TargetLocationKey.ResolveSelectedKey(*BBData);
		WaitTimeKey.ResolveSelectedKey(*BBData);
	}
}

EBTNodeResult::Type UBTTask_SelectNextPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	ALastFPSNPC* NPC = AICon ? Cast<ALastFPSNPC>(AICon->GetPawn()) : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!NPC || !BB || NPC->PatrolPoints.Num() == 0)
	{
		return EBTNodeResult::Failed;
	}

	const int32 Index = FMath::Clamp(NPC->CurrentPatrolIndex, 0, NPC->PatrolPoints.Num() - 1);
	AActor* Point = NPC->PatrolPoints[Index];
	if (!Point)
	{
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsVector(TargetLocationKey.SelectedKeyName, Point->GetActorLocation());
	BB->SetValueAsFloat(WaitTimeKey.SelectedKeyName, NPC->WaitAtPointSec);

	// 다음 인덱스로 (순환). 왕복이 필요하면 여기서 방향 토글 로직 추가.
	NPC->CurrentPatrolIndex = (Index + 1) % NPC->PatrolPoints.Num();

	return EBTNodeResult::Succeeded;
}
