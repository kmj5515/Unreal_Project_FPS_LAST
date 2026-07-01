#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LastFPSPatrolAIController.generated.h"

class UBehaviorTree;

/**
 * 순찰 NPC 전용 AIController.
 * 빙의 시 지정된 BehaviorTree를 실행한다 (Blackboard는 BT 에셋에 연결된 것을 자동 사용).
 * 의사결정/이동은 모두 BT가 담당 — Pawn(ALastFPSNPC)은 몸·상호작용만 책임.
 */
UCLASS()
class LASTFPS_API ALastFPSPatrolAIController : public AAIController
{
	GENERATED_BODY()

protected:
	virtual void OnPossess(APawn* InPawn) override;

	/** 실행할 BehaviorTree (BT_PatrolNPC). BP 디폴트에서 지정. */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|Patrol")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};
