#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyAttack.generated.h"

/** BTTask_EnemyAttack 의 인스턴스별 상태(공격 후 경과 시간). */
struct FBTEnemyAttackMemory
{
	float Elapsed = 0.f;
};

/**
 * AIProfile.AttackAbilityTag 로 지정된 GAS 어빌리티를 발동하는 공격 BT Task.
 *  - 근접/원거리 여부는 어빌리티 구현이 결정(데이터 주도). 이 태스크는 "태그로 발동"만 담당.
 *  - 타깃을 바라본 뒤 ASC->TryActivateAbilitiesByTag 로 공격.
 *  - ReactionDelay(초) 만큼 대기해 공격 간격을 만든 뒤 Succeeded. 쿨다운/실제 데미지는 어빌리티 쪽.
 * 서버에서만 도는 컨트롤러 로직이라 어빌리티도 서버 권위로 발동된다.
 */
UCLASS()
class LASTFPS_API UBTTask_EnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyAttack();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTEnemyAttackMemory); }

protected:
	/** 공격 대상 Actor 키(바라볼 방향). */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetActorKey;
};
