#include "Character/AI/EnvQueryContext_LastFPSTarget.h"

#include "Character/AI/LastFPSEnemyBlackboardKeys.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "GameFramework/Pawn.h"

namespace
{
	AActor* ResolveCombatTarget(UObject* QueryOwner)
	{
		// EQS Owner 는 실행 방식에 따라 폰 또는 컨트롤러일 수 있어 양쪽을 처리한다.
		APawn* Pawn = Cast<APawn>(QueryOwner);
		AController* Controller = Cast<AController>(QueryOwner);
		if (!Controller && Pawn)
		{
			Controller = Pawn->GetController();
		}

		AAIController* AICon = Cast<AAIController>(Controller);
		if (!AICon)
		{
			return nullptr;
		}

		if (const UBlackboardComponent* BB = AICon->GetBlackboardComponent())
		{
			if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(LastFPSEnemyBBKeys::TargetActor)))
			{
				return Target;
			}
		}

		// 폴백: 컨트롤러가 바라보는 FocusActor.
		return AICon->GetFocusActor();
	}
}

void UEnvQueryContext_LastFPSTarget::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	if (AActor* Target = ResolveCombatTarget(QueryInstance.Owner.Get()))
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, Target);
	}
}
