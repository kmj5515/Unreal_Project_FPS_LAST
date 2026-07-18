#include "Character/AI/BTTask_EnemyAttack.h"

#include "Character/AI/LastFPSEnemyBlackboardKeys.h"
#include "Character/LastFPSAIProfile.h"
#include "Character/LastFPSEnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameplayAbilitySpec.h"

UBTTask_EnemyAttack::UBTTask_EnemyAttack()
{
	NodeName = TEXT("Enemy Attack");
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyAttack, TargetActorKey), AActor::StaticClass());
}

void UBTTask_EnemyAttack::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBData = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBData);
	}
}

EBTNodeResult::Type UBTTask_EnemyAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	ALastFPSEnemyCharacter* Enemy = AICon ? Cast<ALastFPSEnemyCharacter>(AICon->GetPawn()) : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Enemy || !BB)
	{
		return EBTNodeResult::Failed;
	}

	const ULastFPSAIProfile* Profile = Enemy->GetAIProfile();
	if (!Profile || !Profile->bCanAttack || !AttackAbilityTag.IsValid())
	{
		// 공격 능력이 없는 적(더미/오브젝트 등)은 이 태스크를 쓰지 않는다.
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// 시야가 막혀 있으면(벽 너머 등) 발사하지 않는다. 실패로 빠져 추격이 재배치하게 한다.
	if (!Target || !AICon->LineOfSightTo(Target))
	{
		return EBTNodeResult::Failed;
	}

	// 태그가 여러 GA와 일치하면 어떤 공격을 기다려야 하는지 모호하므로 설정 오류로 처리한다.
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC)
	{
		return EBTNodeResult::Failed;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(AttackAbilityTag);
	TArray<FGameplayAbilitySpec*> MatchingAbilitySpecs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		AbilityTags,
		MatchingAbilitySpecs,
		true);
	if (MatchingAbilitySpecs.Num() != 1 || !MatchingAbilitySpecs[0])
	{
		return EBTNodeResult::Failed;
	}

	FBTEnemyAttackMemory* Memory = reinterpret_cast<FBTEnemyAttackMemory*>(NodeMemory);
	*Memory = FBTEnemyAttackMemory();
	Memory->ActivatedAbilityHandle = MatchingAbilitySpecs[0]->Handle;
	if (!ASC->TryActivateAbility(Memory->ActivatedAbilityHandle))
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	ALastFPSEnemyCharacter* Enemy = AICon ? Cast<ALastFPSEnemyCharacter>(AICon->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!Enemy || !ASC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FBTEnemyAttackMemory* Memory = reinterpret_cast<FBTEnemyAttackMemory*>(NodeMemory);
	if (!Memory->ActivatedAbilityHandle.IsValid())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FGameplayAbilitySpec* AbilitySpec = ASC->FindAbilitySpecFromHandle(Memory->ActivatedAbilityHandle);
	if (AbilitySpec && AbilitySpec->IsActive())
	{
		return;
	}

	const ULastFPSAIProfile* Profile = Enemy->GetAIProfile();
	const float ReactionDelay = Profile ? Profile->ReactionDelay : 0.f;

	Memory->ElapsedAfterAbilityEnd += DeltaSeconds;

	if (Memory->ElapsedAfterAbilityEnd >= ReactionDelay)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
