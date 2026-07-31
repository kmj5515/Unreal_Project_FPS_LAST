#include "Character/AI/BTService_UpdateCombatTarget.h"

#include "Character/AI/LastFPSCombatTargetRegistry.h"
#include "Character/AI/LastFPSEnemyBlackboardKeys.h"
#include "Character/LastFPSAIProfile.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSEnemyCharacter.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"

namespace
{
	constexpr float InvalidTargetDistance = TNumericLimits<float>::Max();
}

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
		BB->SetValueAsFloat(LastFPSEnemyBBKeys::TargetDistance, InvalidTargetDistance);
		BB->SetValueAsBool(LastFPSEnemyBBKeys::bHasLineOfSight, false);
		BB->SetValueAsBool(LastFPSEnemyBBKeys::bTargetTooClose, false);
	};

	const ULastFPSAIProfile* Profile = Enemy->GetAIProfile();

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(LastFPSEnemyBBKeys::TargetActor));

	// 사망한 타깃은 없는 것으로 본다.
	const ALastFPSCharacterBase* TargetChar = Cast<ALastFPSCharacterBase>(TargetActor);
	if (TargetChar && !TargetChar->IsAlive())
	{
		TargetActor = nullptr;
	}

	// 퍼셉션으로 잡을 수 없는 등록 타깃(방어 대상 등)을 보조 후보로 쓴다.
	// AI 는 "무엇을 지키는 장치인지" 알 필요 없이 등록소에만 질의한다.
	// 프로파일이 없으면 탐색 거리를 알 수 없다. 등록소는 0 을 "무제한"으로 해석하므로
	// 여기서 조회 자체를 건너뛰지 않으면 맵 전역의 장치를 타깃으로 잡게 된다.
	if (!IsValid(TargetActor) && Profile && Profile->DetectionRange > 0.f)
	{
		const UWorld* World = Enemy->GetWorld();
		
		const ULastFPSCombatTargetRegistry* Registry = World 
		? World->GetSubsystem<ULastFPSCombatTargetRegistry>()
		: nullptr;
		
		if (Registry && Registry->HasTargets())
		{
			TargetActor = Registry->FindBestTarget(Enemy->GetActorLocation(),Profile->DetectionRange);
		}
	}

	if (!IsValid(TargetActor))
	{
		ClearTarget();
		return;
	}

	BB->SetValueAsObject(LastFPSEnemyBBKeys::TargetActor, TargetActor);

	const float LoseRange = Profile
		? ((Profile->LoseSightRange > Profile->DetectionRange) ? Profile->LoseSightRange : Profile->DetectionRange)
		: 1600.f;

	const float Distance = FVector::Dist(Enemy->GetActorLocation(), TargetActor->GetActorLocation());

	// 타깃 유지 옵션을 끈 프로파일만 거리 이탈로 추격을 종료한다.
	if ((!Profile || !Profile->bKeepTargetUntilInvalid) && Distance > LoseRange)
	{
		ClearTarget();
		return;
	}

	// 유효 타깃의 마지막 위치, 현재 거리와 시야 상태를 갱신한다.
	BB->SetValueAsVector(LastFPSEnemyBBKeys::TargetLocation, TargetActor->GetActorLocation());
	BB->SetValueAsFloat(LastFPSEnemyBBKeys::TargetDistance, Distance);
	// 시야는 컨트롤러의 LineOfSightTo(폰 시점 기준 트레이스)로 판정.
	BB->SetValueAsBool(LastFPSEnemyBBKeys::bHasLineOfSight, AICon->LineOfSightTo(TargetActor));

	// 카이팅: KeepDistance(>0)보다 가까우면 뒤로 빠질 신호.
	const float KeepDistance = Profile ? Profile->KeepDistance : 0.f;
	BB->SetValueAsBool(LastFPSEnemyBBKeys::bTargetTooClose, KeepDistance > 0.f && Distance < KeepDistance);
}
