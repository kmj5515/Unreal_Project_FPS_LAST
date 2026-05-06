#include "Game/LastFPSLobbyGameMode.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void ALastFPSLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    DebugLobbyFlow(
        FString::Printf(
            TEXT("[Lobby] Player Joined: %s (%d / %d)"),
            *NewPlayer->GetName(),
            GetTotalConnectedPlayers(),
            LobbyStartPlayerCount));

    TryStartMatchFromLobby();
}

void ALastFPSLobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    DebugLobbyFlow(
        FString::Printf(
            TEXT("[Lobby] Player Left: %s (%d / %d)"),
            Exiting ? *Exiting->GetName() : TEXT("Unknown"),
            GetTotalConnectedPlayers(),
            LobbyStartPlayerCount),
        FColor::Yellow);
}

void ALastFPSLobbyGameMode::TryStartMatchFromLobby()
{
    if (bLobbyMatchStartTriggered)
    {
        return;
    }

    const int32 CurrentPlayers = GetTotalConnectedPlayers();
    if (CurrentPlayers < LobbyStartPlayerCount)
    {
        DebugLobbyFlow(
            FString::Printf(
                TEXT("[Lobby] Waiting for players... (%d / %d)"),
                CurrentPlayers,
                LobbyStartPlayerCount),
            FColor::Cyan);
        return;
    }

    bLobbyMatchStartTriggered = true;
    DebugLobbyFlow(
        FString::Printf(
            TEXT("[Lobby] Requirement met. Start match and travel: %s"),
            MatchMapURL.IsEmpty() ? TEXT("(empty map url)") : *MatchMapURL),
        FColor::Green);

    if (MatchMapURL.IsEmpty())
    {
        DebugLobbyFlow(TEXT("[Lobby] MatchMapURL is empty. Travel aborted."), FColor::Red);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->ServerTravel(MatchMapURL);
    }
    else
    {
        DebugLobbyFlow(TEXT("[Lobby] World is null. Travel aborted."), FColor::Red);
    }
}

void ALastFPSLobbyGameMode::DebugLobbyFlow(const FString& Message, FColor Color) const
{
    UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            4.0f,
            Color,
            Message);
    }
}
