#include "Utility/LastFPSCombatAffiliation.h"

#include "GenericTeamAgentInterface.h"
#include "GameFramework/Pawn.h"

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

		const APawn* Pawn = Cast<APawn>(Actor);
		return Pawn ? Cast<IGenericTeamAgentInterface>(Pawn->GetController()) : nullptr;
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
