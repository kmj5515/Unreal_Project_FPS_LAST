#include "Game/LastFPSLobbyGameMode.h"

#include "Game/LastFPSLobbyGameState.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

ALastFPSLobbyGameMode::ALastFPSLobbyGameMode()
{
    GameStateClass = ALastFPSLobbyGameState::StaticClass();
    bUseSeamlessTravel = true;
}

void ALastFPSLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    PlayerReadyMap.Add(NewPlayer, false);
    SyncLobbyStateToGameState();

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
    PlayerReadyMap.Remove(Exiting);
    SyncLobbyStateToGameState();

    DebugLobbyFlow(
        FString::Printf(
            TEXT("[Lobby] Player Left: %s (%d / %d)"),
            Exiting ? *Exiting->GetName() : TEXT("Unknown"),
            GetTotalConnectedPlayers(),
            LobbyStartPlayerCount),
        FColor::Yellow);

    if (bCharacterSelectInProgress && GetTotalConnectedPlayers() < LobbyStartPlayerCount)
    {
        CancelCharacterSelectPhase();
        DebugLobbyFlow(TEXT("[Lobby] Character select cancelled. Not enough players."), FColor::Orange);
    }
}

UClass* ALastFPSLobbyGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    if (LobbyPawnClass)
    {
        return LobbyPawnClass;
    }

    return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ALastFPSLobbyGameMode::TryStartMatchFromLobby()
{
    if (bLobbyMatchStartTriggered || bCharacterSelectInProgress || bTeamIntroInProgress)
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

    StartCharacterSelectPhase();
}

void ALastFPSLobbyGameMode::SyncLobbyStateToGameState()
{
    if (!HasAuthority())
    {
        return;
    }

    ALastFPSLobbyGameState* GS = GetGameState<ALastFPSLobbyGameState>();
    if (!GS)
    {
        return;
    }

    GS->LobbyStartPlayerCount = LobbyStartPlayerCount;
    GS->RemainingCharacterSelectSeconds = RemainingCharacterSelectSeconds;
    GS->bCharacterSelectInProgress = bCharacterSelectInProgress;
    GS->bTeamIntroInProgress = bTeamIntroInProgress;
    GS->bTravelTriggered = bLobbyMatchStartTriggered;
}

void ALastFPSLobbyGameMode::SetPlayerReady(AController* PlayerController, bool bReady)
{
    if (!HasAuthority() || !PlayerController)
    {
        return;
    }

    if (!PlayerReadyMap.Contains(PlayerController))
    {
        return;
    }

    PlayerReadyMap[PlayerController] = bReady;

    DebugLobbyFlow(
        FString::Printf(
            TEXT("[Lobby] Ready changed: %s -> %s"),
            *PlayerController->GetName(),
            bReady ? TEXT("Ready") : TEXT("Not Ready")),
        bReady ? FColor::Green : FColor::Yellow);

    if (bCharacterSelectInProgress && AreAllPlayersReady())
    {
        StartTeamIntroPhase();
    }
}

void ALastFPSLobbyGameMode::StartCharacterSelectPhase()
{
    if (bCharacterSelectInProgress || bTeamIntroInProgress || bLobbyMatchStartTriggered)
    {
        return;
    }

    bCharacterSelectInProgress = true;
    RemainingCharacterSelectSeconds = FMath::Max(0, CharacterSelectSeconds);
    SyncLobbyStateToGameState();

    for (auto& Pair : PlayerReadyMap)
    {
        if (Pair.Key.IsValid())
        {
            Pair.Value = false;
        }
    }

    DebugLobbyFlow(
        FString::Printf(
            TEXT("[Lobby] Character select started (%d second(s))."),
            RemainingCharacterSelectSeconds),
        FColor::Green);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            CharacterSelectTimerHandle,
            this,
            &ALastFPSLobbyGameMode::TickCharacterSelectPhase,
            1.0f,
            true);
    }
    else
    {
        bCharacterSelectInProgress = false;
        DebugLobbyFlow(TEXT("[Lobby] World is null. Character select aborted."), FColor::Red);
    }
}

void ALastFPSLobbyGameMode::TickCharacterSelectPhase()
{
    if (!bCharacterSelectInProgress)
    {
        return;
    }

    const int32 CurrentPlayers = GetTotalConnectedPlayers();
    if (CurrentPlayers < LobbyStartPlayerCount)
    {
        CancelCharacterSelectPhase();
        DebugLobbyFlow(TEXT("[Lobby] Character select cancelled. Not enough players."), FColor::Orange);
        return;
    }

    if (AreAllPlayersReady())
    {
        StartTeamIntroPhase();
        return;
    }

    --RemainingCharacterSelectSeconds;
    SyncLobbyStateToGameState();

    if (RemainingCharacterSelectSeconds > 0)
    {
        DebugLobbyFlow(
            FString::Printf(TEXT("[Lobby] Character select remaining: %d"), RemainingCharacterSelectSeconds),
            FColor::Green);
        return;
    }

    StartTeamIntroPhase();
}

bool ALastFPSLobbyGameMode::AreAllPlayersReady() const
{
    const int32 PlayerCount = GetValidLobbyPlayerCount();
    if (PlayerCount < LobbyStartPlayerCount)
    {
        return false;
    }

    int32 ReadyCount = 0;
    for (const auto& Pair : PlayerReadyMap)
    {
        if (Pair.Key.IsValid() && Pair.Value)
        {
            ++ReadyCount;
        }
    }

    return ReadyCount >= PlayerCount;
}

int32 ALastFPSLobbyGameMode::GetValidLobbyPlayerCount() const
{
    int32 Count = 0;
    for (const auto& Pair : PlayerReadyMap)
    {
        if (Pair.Key.IsValid())
        {
            ++Count;
        }
    }
    return Count;
}

void ALastFPSLobbyGameMode::ClearLobbyTimers()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CharacterSelectTimerHandle);
        World->GetTimerManager().ClearTimer(TeamIntroTimerHandle);
    }
}

void ALastFPSLobbyGameMode::ResetLobbyFlowState()
{
    bCharacterSelectInProgress = false;
    bTeamIntroInProgress = false;
    bLobbyMatchStartTriggered = false;
    RemainingCharacterSelectSeconds = 0;
}

void ALastFPSLobbyGameMode::CancelCharacterSelectPhase()
{
    ClearLobbyTimers();
    bCharacterSelectInProgress = false;
    bLobbyMatchStartTriggered = false;
    RemainingCharacterSelectSeconds = 0;
    SyncLobbyStateToGameState();
}

void ALastFPSLobbyGameMode::StartTeamIntroPhase()
{
    if (bTeamIntroInProgress || bLobbyMatchStartTriggered)
    {
        return;
    }

    ClearLobbyTimers();
    bCharacterSelectInProgress = false;
    bTeamIntroInProgress = true;
    SyncLobbyStateToGameState();

    DebugLobbyFlow(
        FString::Printf(
            TEXT("[Lobby] Team intro started (%.1f second(s))."),
            TeamIntroSeconds),
        FColor::Blue);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TeamIntroTimerHandle,
            this,
            &ALastFPSLobbyGameMode::FinishTeamIntroPhase,
            FMath::Max(0.1f, TeamIntroSeconds),
            false);
    }
    else
    {
        bTeamIntroInProgress = false;
        DebugLobbyFlow(TEXT("[Lobby] World is null. Team intro aborted."), FColor::Red);
    }
}

void ALastFPSLobbyGameMode::FinishTeamIntroPhase()
{
    bTeamIntroInProgress = false;
    SyncLobbyStateToGameState();
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
        ClearLobbyTimers();
        ResetLobbyFlowState();
        SyncLobbyStateToGameState();
        DebugLobbyFlow(TEXT("[Lobby] MatchMapURL is empty. Travel aborted."), FColor::Red);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        ClearLobbyTimers();
        bCharacterSelectInProgress = false;
        bTeamIntroInProgress = false;
        bLobbyMatchStartTriggered = true;
        SyncLobbyStateToGameState();
        World->ServerTravel(MatchMapURL);
    }
    else
    {
        ResetLobbyFlowState();
        SyncLobbyStateToGameState();
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
