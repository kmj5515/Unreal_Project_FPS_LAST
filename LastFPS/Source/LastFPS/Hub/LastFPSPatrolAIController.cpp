#include "Hub/LastFPSPatrolAIController.h"

#include "BehaviorTree/BehaviorTree.h"

void ALastFPSPatrolAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		// RunBehaviorTree가 BlackboardComponent도 BT 에셋의 BB 데이터로 초기화한다.
		RunBehaviorTree(BehaviorTreeAsset);
	}
}
