#include "Game/LastFPSLobbyGameState.h"

#include "Net/UnrealNetwork.h"

void ALastFPSLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALastFPSLobbyGameState, LobbyStartPlayerCount);
    DOREPLIFETIME(ALastFPSLobbyGameState, RemainingCharacterSelectSeconds);
    DOREPLIFETIME(ALastFPSLobbyGameState, bCharacterSelectInProgress);
    DOREPLIFETIME(ALastFPSLobbyGameState, bTeamIntroInProgress);
    DOREPLIFETIME(ALastFPSLobbyGameState, bTravelTriggered);
}

