#include "Utility/LastFPSCombatAffiliation.h"

#include "GenericTeamAgentInterface.h"
#include "GameFramework/Pawn.h"
#include "Utility/LastFPSTeamComponent.h"

namespace
{
	const IGenericTeamAgentInterface* ResolveTeamAgent(const AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		if (const IGenericTeamAgentInterface* ActorTeamAgent = Cast<IGenericTeamAgentInterface>(Actor))
		{
			return ActorTeamAgent;
		}

		// 폰은 컨트롤러가 진영을 소유한다. 빙의 중이 아니면 아래 컴포넌트 경로로 내려간다.
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (const IGenericTeamAgentInterface* ControllerTeamAgent =
				Cast<IGenericTeamAgentInterface>(Pawn->GetController()))
			{
				return ControllerTeamAgent;
			}
		}

		// 컨트롤러가 없는 액터(수호 목표·파괴물 등)는 진영 컴포넌트로 진영을 밝힌다.
		// 이 경로가 없으면 대상이 "진영 미상"이 되어 아군 사격이 그대로 통과한다.
		return Actor->FindComponentByClass<ULastFPSTeamComponent>();
	}
}

bool LastFPSCombatAffiliation::AreFriendlyActors(const AActor* SourceActor, const AActor* TargetActor)
{
	if (!IsValid(SourceActor) || !IsValid(TargetActor))
	{
		return false;
	}

	const IGenericTeamAgentInterface* SourceTeamAgent = ResolveTeamAgent(SourceActor);
	const IGenericTeamAgentInterface* TargetTeamAgent = ResolveTeamAgent(TargetActor);
	if (!SourceTeamAgent || !TargetTeamAgent)
	{
		return false;
	}

	const FGenericTeamId SourceTeamId = SourceTeamAgent->GetGenericTeamId();
	const FGenericTeamId TargetTeamId = TargetTeamAgent->GetGenericTeamId();
	return SourceTeamId != FGenericTeamId::NoTeam && SourceTeamId == TargetTeamId;
}
