#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateCombatTarget.generated.h"

/**
 * 매 틱 Blackboard 의 교전 상태를 갱신하는 BT Service.
 *  - TargetActor 유효성 검사: null/사망/LoseSightRange 초과면 타깃을 해제(Clear).
 *  - 살아있는 타깃이면 TargetLocation과 TargetDistance를 갱신한다.
	* 탐지 해제 거리와 카이팅 거리는 Pawn의 AIProfile에서 읽는다.
 *
 * BT 배치: 전투 서브트리 상단에 Service 로 부착.
 */
UCLASS()
class LASTFPS_API UBTService_UpdateCombatTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateCombatTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
