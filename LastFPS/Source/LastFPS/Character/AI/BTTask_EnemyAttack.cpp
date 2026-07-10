#include "Character/AI/BTTask_EnemyAttack.h"

#include "Character/AI/LastFPSEnemyBlackboardKeys.h"
#include "Character/LastFPSAIProfile.h"
#include "Character/LastFPSEnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

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
	if (!Profile || !Profile->bCanAttack || !Profile->AttackAbilityTag.IsValid())
	{
		// 공격 능력이 없는 적(더미/오브젝트 등)은 이 태스크를 쓰지 않는다.
		return EBTNodeResult::Failed;
	}

	// 타깃을 바라본다(발사/스윙 방향 정렬).
	if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName)))
	{
		AICon->SetFocus(Target);
	}

	// AttackAbilityTag 로 어빌리티 발동. 쿨다운이면 활성화 실패할 수 있으나,
	// 그 경우에도 ReactionDelay 만큼 사거리에서 대기했다가 재시도하도록 InProgress 로 진행한다.
	if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
	{
		FGameplayTagContainer AbilityTags;
		AbilityTags.AddTag(Profile->AttackAbilityTag);
		ASC->TryActivateAbilitiesByTag(AbilityTags);
	}

	FBTEnemyAttackMemory* Memory = reinterpret_cast<FBTEnemyAttackMemory*>(NodeMemory);
	Memory->Elapsed = 0.f;

	// ReactionDelay 가 0 이하이면 즉시 종료(다음 틱에 상위 트리가 재판단).
	if (Profile->ReactionDelay <= 0.f)
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	ALastFPSEnemyCharacter* Enemy = AICon ? Cast<ALastFPSEnemyCharacter>(AICon->GetPawn()) : nullptr;
	if (!Enemy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 대기 중에도 타깃을 계속 바라본다.
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName)))
		{
			AICon->SetFocus(Target);
		}
	}

	const ULastFPSAIProfile* Profile = Enemy->GetAIProfile();
	const float ReactionDelay = Profile ? Profile->ReactionDelay : 0.f;

	FBTEnemyAttackMemory* Memory = reinterpret_cast<FBTEnemyAttackMemory*>(NodeMemory);
	Memory->Elapsed += DeltaSeconds;

	if (Memory->Elapsed >= ReactionDelay)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
