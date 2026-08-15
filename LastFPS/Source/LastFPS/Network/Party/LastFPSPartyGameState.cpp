#include "LastFPSPartyGameState.h"
#include "LastFPSPartyPlayerState.h"

ALastFPSPartyGameState::ALastFPSPartyGameState()
{
}

void ALastFPSPartyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	OnPartyMembersUpdated.Broadcast();
}

void ALastFPSPartyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	OnPartyMembersUpdated.Broadcast();
}

TArray<ALastFPSPartyPlayerState*> ALastFPSPartyGameState::GetPartyMembers() const
{
	TArray<ALastFPSPartyPlayerState*> Members;
	for (APlayerState* PS : PlayerArray)
	{
		if (ALastFPSPartyPlayerState* PartyPS = Cast<ALastFPSPartyPlayerState>(PS))
		{
			Members.Add(PartyPS);
		}
	}
	return Members;
}
