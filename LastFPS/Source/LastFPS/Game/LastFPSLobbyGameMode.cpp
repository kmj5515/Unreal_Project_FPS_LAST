#include "Game/LastFPSLobbyGameMode.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

    if (bCountdownInProgress && GetTotalConnectedPlayers() < LobbyStartPlayerCount)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(MatchStartCountdownTimerHandle);
        }

        bCountdownInProgress = false;
        bLobbyMatchStartTriggered = false;
        RemainingCountdownSeconds = 0;

        DebugLobbyFlow(TEXT("[Lobby] Countdown cancelled. Not enough players."), FColor::Orange);
    }
}

void ALastFPSLobbyGameMode::TryStartMatchFromLobby()
{
    if (bLobbyMatchStartTriggered || bCountdownInProgress)
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

    StartMatchCountdown();
}

void ALastFPSLobbyGameMode::StartMatchCountdown()
{
    if (bCountdownInProgress || bLobbyMatchStartTriggered)
    {
        return;
    }

    bCountdownInProgress = true;
    RemainingCountdownSeconds = FMath::Max(0, MatchStartCountdownSeconds);

    DebugLobbyFlow(
        FString::Printf(
            TEXT("[Lobby] Requirement met. Match starts in %d second(s)."),
            RemainingCountdownSeconds),
        FColor::Green);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            MatchStartCountdownTimerHandle,
            this,
            &ALastFPSLobbyGameMode::TickMatchCountdown,
            1.0f,
            true);
    }
    else
    {
        bCountdownInProgress = false;
        DebugLobbyFlow(TEXT("[Lobby] World is null. Countdown aborted."), FColor::Red);
    }
}

void ALastFPSLobbyGameMode::TickMatchCountdown()
{
    if (!bCountdownInProgress)
    {
        return;
    }

    const int32 CurrentPlayers = GetTotalConnectedPlayers();
    if (CurrentPlayers < LobbyStartPlayerCount)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(MatchStartCountdownTimerHandle);
        }

        bCountdownInProgress = false;
        bLobbyMatchStartTriggered = false;
        RemainingCountdownSeconds = 0;
        DebugLobbyFlow(TEXT("[Lobby] Countdown cancelled. Not enough players."), FColor::Orange);
        return;
    }

    --RemainingCountdownSeconds;

    if (RemainingCountdownSeconds > 0)
    {
        DebugLobbyFlow(
            FString::Printf(TEXT("[Lobby] Match starts in %d..."), RemainingCountdownSeconds),
            FColor::Green);
        return;
    }

    ExecuteMatchTravel();
}

void ALastFPSLobbyGameMode::ExecuteMatchTravel()
{
    DebugLobbyFlow(
        FString::Printf(
            TEXT("[Lobby] Requirement met. Start match and travel: %s"),
            MatchMapURL.IsEmpty() ? TEXT("(empty map url)") : *MatchMapURL),
        FColor::Green);

    if (MatchMapURL.IsEmpty())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(MatchStartCountdownTimerHandle);
        }
        bCountdownInProgress = false;
        bLobbyMatchStartTriggered = false;
        DebugLobbyFlow(TEXT("[Lobby] MatchMapURL is empty. Travel aborted."), FColor::Red);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MatchStartCountdownTimerHandle);
        bCountdownInProgress = false;
        bLobbyMatchStartTriggered = true;
        World->ServerTravel(MatchMapURL);
    }
    else
    {
        bCountdownInProgress = false;
        bLobbyMatchStartTriggered = false;
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
