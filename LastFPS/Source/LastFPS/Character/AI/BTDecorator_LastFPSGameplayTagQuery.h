#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_LastFPSGameplayTagQuery.generated.h"

class UBehaviorTree;
class UBehaviorTreeComponent;

/**
 * Blackboard의 Actor가 소유한 ASC 태그를 쿼리하고, 관련 태그 변경 시 BT 흐름을 즉시 재평가한다.
 * 특정 적이나 상태를 알지 않는 범용 데코레이터로 유지하여 모든 GAS 기반 AI가 재사용할 수 있다.
 */
UCLASS()
class LASTFPS_API UBTDecorator_LastFPSGameplayTagQuery : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_LastFPSGameplayTagQuery();

	virtual bool CalculateRawConditionValue(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		EBTMemoryClear::Type CleanupType) const override;

protected:
	virtual void OnBecomeRelevant(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category="Gameplay Tag Query")
	FBlackboardKeySelector ActorToCheck;

	UPROPERTY(EditAnywhere, Category="Gameplay Tag Query")
	FGameplayTagQuery GameplayTagQuery;

private:
	void OnObservedTagChanged(
		FGameplayTag ChangedTag,
		int32 NewCount,
		TWeakObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent);
};
