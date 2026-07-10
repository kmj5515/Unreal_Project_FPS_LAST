#include "Character/AI/BTService_UpdateCombatTarget.h"

#include "Character/AI/LastFPSEnemyBlackboardKeys.h"
#include "Character/LastFPSAIProfile.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSEnemyCharacter.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_UpdateCombatTarget::UBTService_UpdateCombatTarget()
{
	NodeName = TEXT("Update Combat Target");
	Interval = 0.15f;
	RandomDeviation = 0.05f;
	bNotifyTick = true;
}

void UBTService_UpdateCombatTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AICon = OwnerComp.GetAIOwner();
	ALastFPSEnemyCharacter* Enemy = AICon ? Cast<ALastFPSEnemyCharacter>(AICon->GetPawn()) : nullptr;
	if (!BB || !Enemy)
	{
		return;
	}

	auto ClearTarget = [BB]()
	{
		BB->ClearValue(LastFPSEnemyBBKeys::TargetActor);
		BB->SetValueAsBool(LastFPSEnemyBBKeys::bInAttackRange, false);
	};

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(LastFPSEnemyBBKeys::TargetActor));
	if (!TargetActor)
	{
		ClearTarget();
		return;
	}

	// 사망한 타깃은 즉시 해제.
	const ALastFPSCharacterBase* TargetChar = Cast<ALastFPSCharacterBase>(TargetActor);
	if (TargetChar && !TargetChar->IsAlive())
	{
		ClearTarget();
		return;
	}

	const ULastFPSAIProfile* Profile = Enemy->GetAIProfile();
	const float AttackRange = Profile ? Profile->AttackRange : 200.f;
	const float LoseRange = Profile
		? ((Profile->LoseSightRange > Profile->DetectionRange) ? Profile->LoseSightRange : Profile->DetectionRange)
		: 1600.f;

	const float Distance = FVector::Dist(Enemy->GetActorLocation(), TargetActor->GetActorLocation());

	// 놓치는 거리를 벗어나면 타깃 해제(추격 종료).
	if (Distance > LoseRange)
	{
		ClearTarget();
		return;
	}

	// 유효 타깃: 마지막 위치와 사거리 진입 여부 갱신.
	BB->SetValueAsVector(LastFPSEnemyBBKeys::TargetLocation, TargetActor->GetActorLocation());
	BB->SetValueAsBool(LastFPSEnemyBBKeys::bInAttackRange, Distance <= AttackRange);
}
