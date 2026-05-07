#include "Game/LastFPSPlayerController.h"

#include "Engine/World.h"
#include "Game/LastFPSLobbyGameMode.h"

void ALastFPSPlayerController::SetLobbyReady(bool bReady)
{
    bLobbyReady = bReady;
    ServerSetLobbyReady(bReady);
}

void ALastFPSPlayerController::ServerSetLobbyReady_Implementation(bool bReady)
{
    bLobbyReady = bReady;

    if (UWorld* World = GetWorld())
    {
        if (ALastFPSLobbyGameMode* LobbyGameMode = Cast<ALastFPSLobbyGameMode>(World->GetAuthGameMode()))
        {
            LobbyGameMode->SetPlayerReady(this, bReady);
        }
    }
}
