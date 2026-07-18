#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "BTTask_EnemyAttack.generated.h"

/** 활성화한 GA와 종료 후 대기 시간을 저장하는 Task 인스턴스별 상태다. */
struct FBTEnemyAttackMemory
{
	FGameplayAbilitySpecHandle ActivatedAbilityHandle;
	float ElapsedAfterAbilityEnd = 0.f;
};

/**
 * Task에 지정된 Gameplay Tag로 GAS 어빌리티를 발동하는 공격 BT Task다.
 *  - 공격 종류는 Behavior Tree가 결정하고 이 Task는 태그 기반 활성화만 담당한다.
 *  - 지정된 GA를 활성화하고 종료될 때까지 대기한다.
 *  - GA 종료 후 ReactionDelay만큼 추가 대기해 공격 간격을 보장한다.
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

	/** 이 Task가 실행할 GA의 Asset Tag다. */
	UPROPERTY(EditAnywhere, Category="Attack", meta=(Categories="Ability.Enemy"))
	FGameplayTag AttackAbilityTag;
};
