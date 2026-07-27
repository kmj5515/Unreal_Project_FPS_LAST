#include "Character/AI/BTDecorator_LastFPSGameplayTagQuery.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Actor.h"
#include "GameplayTagAssetInterface.h"

namespace
{
struct FLastFPSGameplayTagQueryDecoratorMemory
{
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	TArray<TPair<FGameplayTag, FDelegateHandle>> TagEventHandles;
};

void UnregisterTagEvents(FLastFPSGameplayTagQueryDecoratorMemory& Memory)
{
	if (UAbilitySystemComponent* AbilitySystem = Memory.AbilitySystem.Get())
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& Entry : Memory.TagEventHandles)
		{
			AbilitySystem
				->RegisterGameplayTagEvent(
					Entry.Key,
					EGameplayTagEventType::AnyCountChange)
				.Remove(Entry.Value);
		}
	}

	Memory.TagEventHandles.Reset();
	Memory.AbilitySystem.Reset();
}
}

UBTDecorator_LastFPSGameplayTagQuery::UBTDecorator_LastFPSGameplayTagQuery()
{
	NodeName = TEXT("LastFPS Gameplay Tag Query");
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();

	ActorToCheck.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(
			UBTDecorator_LastFPSGameplayTagQuery,
			ActorToCheck),
		AActor::StaticClass());
	ActorToCheck.SelectedKeyName = FBlackboard::KeySelf;
}

bool UBTDecorator_LastFPSGameplayTagQuery::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard || GameplayTagQuery.IsEmpty())
	{
		return false;
	}

	const AActor* Actor = Cast<AActor>(
		Blackboard->GetValue<UBlackboardKeyType_Object>(
			ActorToCheck.GetSelectedKeyID()));
	const IGameplayTagAssetInterface* TagAsset =
		Cast<IGameplayTagAssetInterface>(Actor);
	if (!TagAsset)
	{
		return false;
	}

	FGameplayTagContainer OwnedTags;
	TagAsset->GetOwnedGameplayTags(OwnedTags);
	return GameplayTagQuery.Matches(OwnedTags);
}

FString UBTDecorator_LastFPSGameplayTagQuery::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s: %s"),
		*Super::GetStaticDescription(),
		*GameplayTagQuery.GetDescription());
}

void UBTDecorator_LastFPSGameplayTagQuery::InitializeFromAsset(
	UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		ActorToCheck.ResolveSelectedKey(*BlackboardAsset);
	}
	else
	{
		ActorToCheck.InvalidateResolvedKey();
	}
}

uint16 UBTDecorator_LastFPSGameplayTagQuery::GetInstanceMemorySize() const
{
	return sizeof(FLastFPSGameplayTagQueryDecoratorMemory);
}

void UBTDecorator_LastFPSGameplayTagQuery::InitializeMemory(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	const EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FLastFPSGameplayTagQueryDecoratorMemory>(
		NodeMemory,
		InitType);
}

void UBTDecorator_LastFPSGameplayTagQuery::CleanupMemory(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	const EBTMemoryClear::Type CleanupType) const
{
	FLastFPSGameplayTagQueryDecoratorMemory* Memory =
		CastInstanceNodeMemory<FLastFPSGameplayTagQueryDecoratorMemory>(
			NodeMemory);
	UnregisterTagEvents(*Memory);
	CleanupNodeMemory<FLastFPSGameplayTagQueryDecoratorMemory>(
		NodeMemory,
		CleanupType);
}

void UBTDecorator_LastFPSGameplayTagQuery::OnBecomeRelevant(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	const AActor* Actor = Cast<AActor>(
		Blackboard->GetValue<UBlackboardKeyType_Object>(
			ActorToCheck.GetSelectedKeyID()));
	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
	if (!AbilitySystem)
	{
		return;
	}

	FLastFPSGameplayTagQueryDecoratorMemory* Memory =
		CastInstanceNodeMemory<FLastFPSGameplayTagQueryDecoratorMemory>(
			NodeMemory);
	UnregisterTagEvents(*Memory);
	Memory->AbilitySystem = AbilitySystem;

	TArray<FGameplayTag> QueryTags;
	GameplayTagQuery.GetGameplayTagArray(QueryTags);
	for (const FGameplayTag& Tag : QueryTags)
	{
		if (!Tag.IsValid())
		{
			continue;
		}

		const FDelegateHandle Handle = AbilitySystem
			->RegisterGameplayTagEvent(
				Tag,
				EGameplayTagEventType::AnyCountChange)
			.AddUObject(
				this,
				&ThisClass::OnObservedTagChanged,
				TWeakObjectPtr<UBehaviorTreeComponent>(&OwnerComp));
		Memory->TagEventHandles.Emplace(Tag, Handle);
	}
}

void UBTDecorator_LastFPSGameplayTagQuery::OnCeaseRelevant(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	FLastFPSGameplayTagQueryDecoratorMemory* Memory =
		CastInstanceNodeMemory<FLastFPSGameplayTagQueryDecoratorMemory>(
			NodeMemory);
	UnregisterTagEvents(*Memory);
}

void UBTDecorator_LastFPSGameplayTagQuery::OnObservedTagChanged(
	FGameplayTag ChangedTag,
	int32 NewCount,
	TWeakObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent)
{
	if (UBehaviorTreeComponent* BehaviorTree = BehaviorTreeComponent.Get())
	{
		ConditionalFlowAbort(
			*BehaviorTree,
			EBTDecoratorAbortRequest::ConditionResultChanged);
	}
}
