#include "Character/AI/LastFPSEnemyAIController.h"

#include "Character/AI/LastFPSEnemyBlackboardKeys.h"
#include "Character/LastFPSAIProfile.h"
#include "Character/LastFPSEnemyCharacter.h"
#include "Character/LastFPSHero.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Utility/LastFPSTeamTypes.h"

ALastFPSEnemyAIController::ALastFPSEnemyAIController()
{
	SetGenericTeamId(FGenericTeamId(static_cast<uint8>(ELastFPSTeam::Enemy)));

	EnemyPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EnemyPerception"));
	SightConfig     = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	// 팀 태도가 적대적인 대상만 감지 후보로 받는다.
	SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals   = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->PeripheralVisionAngleDegrees = 70.f;
	SightConfig->SetMaxAge(5.f);

	EnemyPerception->ConfigureSense(*SightConfig);
	EnemyPerception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*EnemyPerception);
}

AActor* ALastFPSEnemyAIController::GetCombatTargetActor() const
{
	const UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return nullptr;
	}

	return Cast<AActor>(BlackboardComponent->GetValueAsObject(LastFPSEnemyBBKeys::TargetActor));
}

void ALastFPSEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ConfigureSightFromProfile();

	if (EnemyPerception)
	{
		EnemyPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ALastFPSEnemyAIController::OnPerceptionUpdated);
	}

	if (BehaviorTreeAsset)
	{
		// RunBehaviorTree 가 BlackboardComponent 도 BT 에 연결된 BB 데이터로 초기화한다.
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void ALastFPSEnemyAIController::OnUnPossess()
{
	if (EnemyPerception)
	{
		EnemyPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALastFPSEnemyAIController::OnPerceptionUpdated);
	}

	Super::OnUnPossess();
}

void ALastFPSEnemyAIController::ConfigureSightFromProfile()
{
	const ALastFPSEnemyCharacter* Enemy = Cast<ALastFPSEnemyCharacter>(GetPawn());
	const ULastFPSAIProfile* Profile = Enemy ? Enemy->GetAIProfile() : nullptr;
	if (!Profile || !SightConfig || !EnemyPerception)
	{
		return;
	}

	const float SightRadius = Profile->DetectionRange;
	// 놓치는 거리는 감지 거리보다 같거나 크게(히스테리시스). 0 이하이거나 더 작으면 감지 거리로 폴백.
	const float LoseRadius = (Profile->LoseSightRange > SightRadius) ? Profile->LoseSightRange : SightRadius;

	SightConfig->SightRadius     = SightRadius;
	SightConfig->LoseSightRadius = LoseRadius;

	// 변경한 설정을 퍼셉션에 다시 반영.
	EnemyPerception->ConfigureSense(*SightConfig);
}

void ALastFPSEnemyAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 플레이어(Hero)만 교전 대상으로 삼는다. 다른 적/NPC 는 무시.
	if (!Actor || !Actor->IsA<ALastFPSHero>())
	{
		return;
	}

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// 새로 포착: 타깃과 마지막 위치를 기록. 사거리 판정/해제는 BTService 가 매 틱 갱신.
		BB->SetValueAsObject(LastFPSEnemyBBKeys::TargetActor, Actor);
		BB->SetValueAsVector(LastFPSEnemyBBKeys::TargetLocation, Actor->GetActorLocation());
	}
	// 소실(WasSuccessfullySensed == false)은 여기서 즉시 지우지 않는다.
	// 마지막 위치까지 추격하도록 두고, 최종 해제는 서비스가 LoseSightRange/사망 기준으로 판단한다.
}
