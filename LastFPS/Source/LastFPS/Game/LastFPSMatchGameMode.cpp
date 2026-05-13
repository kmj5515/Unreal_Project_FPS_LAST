#include "Game/LastFPSMatchGameMode.h"

#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Character/LastFPSCharacterBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Game/LastFPSMatchGameState.h"
#include "Game/LastFPSPlayerState.h"

ALastFPSMatchGameMode::ALastFPSMatchGameMode()
{
    bUseSeamlessTravel = true;
    GameStateClass = ALastFPSMatchGameState::StaticClass();
}

void ALastFPSMatchGameMode::RefillMatchPlayerStartDeck(UWorld* World)
{
    MatchPlayerStartDeck.Empty();

    TArray<APlayerStart*> Starts;
    for (TActorIterator<APlayerStart> It(World); It; ++It)
    {
        Starts.Add(*It);
    }

    for (APlayerStart* PS : Starts)
    {
        if (PS)
        {
            MatchPlayerStartDeck.Add(PS);
        }
    }

    for (int32 i = MatchPlayerStartDeck.Num() - 1; i > 0; --i)
    {
        const int32 j = FMath::RandRange(0, i);
        MatchPlayerStartDeck.Swap(i, j);
    }
}

AActor* ALastFPSMatchGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return Super::ChoosePlayerStart_Implementation(Player);
    }

    TArray<APlayerStart*> AllStarts;
    for (TActorIterator<APlayerStart> It(World); It; ++It)
    {
        AllStarts.Add(*It);
    }

    if (AllStarts.Num() == 0)
    {
        return Super::ChoosePlayerStart_Implementation(Player);
    }

    MatchPlayerStartDeck.RemoveAll(
        [&AllStarts](const TWeakObjectPtr<APlayerStart>& Ptr)
        {
            return !Ptr.IsValid() || !AllStarts.Contains(Ptr.Get());
        });

    if (MatchPlayerStartDeck.Num() == 0)
    {
        RefillMatchPlayerStartDeck(World);
    }

    while (MatchPlayerStartDeck.Num() > 0)
    {
        const TWeakObjectPtr<APlayerStart> Back = MatchPlayerStartDeck.Pop(false);
        if (APlayerStart* Chosen = Back.Get())
        {
            return Chosen;
        }
    }

    return Super::ChoosePlayerStart_Implementation(Player);
}

void ALastFPSMatchGameMode::BeginPlay()
{
    Super::BeginPlay();

    MatchPlayerStartDeck.Empty();

    DebugFlow(TEXT("[Match] MatchGameMode BeginPlay"));
    StartDropIntroPhase();
}

void ALastFPSMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    DebugFlow(FString::Printf(
        TEXT("[Match] Player Joined: %s (Total: %d)"),
        *NewPlayer->GetName(),
        GetTotalConnectedPlayers()),
        FColor::Cyan);
}

void ALastFPSMatchGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    PendingRespawnControllers.Remove(Exiting);
}

void ALastFPSMatchGameMode::StartDropIntroPhase()
{
    if (bDropIntroInProgress || bMatchEnded)
    {
        return;
    }

    bDropIntroInProgress = true;

    if (ALastFPSMatchGameState* MGS = GetGameState<ALastFPSMatchGameState>())
    {
        MGS->Auth_SetDropIntroActive(true);
    }

    DebugFlow(
        FString::Printf(TEXT("[Match] Drop intro started (%.1f second(s))."), DropIntroSeconds),
        FColor::Blue);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            DropIntroTimerHandle,
            this,
            &ALastFPSMatchGameMode::FinishDropIntroPhase,
            FMath::Max(0.1f, DropIntroSeconds),
            false);
    }
}

void ALastFPSMatchGameMode::FinishDropIntroPhase()
{
    if (bMatchEnded)
    {
        return;
    }

    bDropIntroInProgress = false;

    if (ALastFPSMatchGameState* MGS = GetGameState<ALastFPSMatchGameState>())
    {
        MGS->Auth_SetDropIntroActive(false);
    }

    MatchStartServerTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    DebugFlow(
        FString::Printf(
            TEXT("[Match] Drop intro finished. Match started (%d sec, kill limit %d)."),
            MatchDurationSeconds,
            MatchKillLimit),
        FColor::Green);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            MatchRuleTimerHandle,
            this,
            &ALastFPSMatchGameMode::TickMatchRuleCheck,
            0.25f,
            true);
    }
}

void ALastFPSMatchGameMode::TickMatchRuleCheck()
{
    if (bMatchEnded)
    {
        return;
    }

    ScheduleRespawnForDeadPlayers();

    if (UWorld* World = GetWorld())
    {
        if (ALastFPSMatchGameState* MatchGS = World->GetGameState<ALastFPSMatchGameState>())
        {
            const float Elapsed = World->GetTimeSeconds() - MatchStartServerTimeSeconds;
            MatchGS->Auth_SetMatchTimeRemaining(MatchDurationSeconds - Elapsed);
        }
    }

    FString EndReason;
    APlayerState* Winner = nullptr;
    if (IsMatchEndConditionMet(EndReason, Winner))
    {
        EndMatch(EndReason, Winner);
    }
}

void ALastFPSMatchGameMode::ScheduleRespawnForDeadPlayers()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UpdateRespawnSchedule(World);
    ProcessReadyRespawns(World);
}

void ALastFPSMatchGameMode::UpdateRespawnSchedule(UWorld* World)
{
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        ALastFPSCharacterBase* Character = PC ? Cast<ALastFPSCharacterBase>(PC->GetPawn()) : nullptr;
        if (!PC || !Character)
        {
            continue;
        }

        if (Character->IsAlive())
        {
            PendingRespawnControllers.Remove(PC);
            continue;
        }

        if (!PendingRespawnControllers.Contains(PC))
        {
            const float RespawnAt = World->GetTimeSeconds() + RespawnDelaySeconds;
            PendingRespawnControllers.Add(PC, RespawnAt);

            DebugFlow(
                FString::Printf(
                    TEXT("[Match] Respawn scheduled: %s in %.1f second(s)."),
                    *PC->GetName(),
                    RespawnDelaySeconds),
                FColor::Yellow);
        }
    }
}

void ALastFPSMatchGameMode::ProcessReadyRespawns(UWorld* World)
{
    TArray<TWeakObjectPtr<AController>> ReadyToRespawn;
    for (const auto& Pair : PendingRespawnControllers)
    {
        if (!Pair.Key.IsValid())
        {
            continue;
        }

        if (World->GetTimeSeconds() >= Pair.Value)
        {
            ReadyToRespawn.Add(Pair.Key);
        }
    }

    for (const TWeakObjectPtr<AController>& WeakController : ReadyToRespawn)
    {
        if (AController* Controller = WeakController.Get())
        {
            RespawnController(Controller);
        }
        PendingRespawnControllers.Remove(WeakController);
    }
}

void ALastFPSMatchGameMode::RespawnController(AController* ControllerToRespawn)
{
    if (!ControllerToRespawn || bMatchEnded)
    {
        return;
    }

    if (APawn* OldPawn = ControllerToRespawn->GetPawn())
    {
        OldPawn->Destroy();
    }

    RestartPlayer(ControllerToRespawn);

    if (ALastFPSPlayerState* PS = ControllerToRespawn->GetPlayerState<ALastFPSPlayerState>())
    {
        if (ULastFPSAttributeSet* AttrSet = PS->GetAttributeSet())
        {
            AttrSet->SetHealth(AttrSet->GetMaxHealth());
            AttrSet->SetStamina(AttrSet->GetMaxStamina());
        }
    }

    DebugFlow(FString::Printf(TEXT("[Match] Respawned: %s"), *ControllerToRespawn->GetName()), FColor::Cyan);
}

bool ALastFPSMatchGameMode::IsMatchEndConditionMet(FString& OutReason, APlayerState*& OutWinner) const
{
    return CheckTimeLimit(OutReason, OutWinner) || CheckKillLimit(OutReason, OutWinner);
}

bool ALastFPSMatchGameMode::CheckTimeLimit(FString& OutReason, APlayerState*& OutWinner) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const float ElapsedSeconds = World->GetTimeSeconds() - MatchStartServerTimeSeconds;
    if (ElapsedSeconds >= MatchDurationSeconds)
    {
        OutReason = FString::Printf(TEXT("Time limit reached (%d sec)"), MatchDurationSeconds);
        OutWinner = DetermineLeadingPlayer();
        return true;
    }

    return false;
}

bool ALastFPSMatchGameMode::CheckKillLimit(FString& OutReason, APlayerState*& OutWinner) const
{
    if (!GameState)
    {
        return false;
    }

    for (APlayerState* PS : GameState->PlayerArray)
    {
        const ALastFPSPlayerState* LastPS = Cast<ALastFPSPlayerState>(PS);
        if (LastPS && LastPS->GetStatKills() >= MatchKillLimit)
        {
            OutReason = FString::Printf(
                TEXT("Kill limit reached by %s (%d kills)"),
                *LastPS->GetPlayerName(),
                LastPS->GetStatKills());
            OutWinner = PS;
            return true;
        }
    }

    return false;
}

APlayerState* ALastFPSMatchGameMode::DetermineLeadingPlayer() const
{
    if (!GameState)
    {
        return nullptr;
    }

    APlayerState* Best         = nullptr;
    int32         BestKills    = -1;
    int32         BestDeaths   = INT32_MAX;
    bool          bTie         = false;

    for (APlayerState* PS : GameState->PlayerArray)
    {
        const ALastFPSPlayerState* LastPS = Cast<ALastFPSPlayerState>(PS);
        if (!LastPS)
        {
            continue;
        }

        const int32 Kills  = LastPS->GetStatKills();
        const int32 Deaths = LastPS->GetStatDeaths();

        if (Kills > BestKills || (Kills == BestKills && Deaths < BestDeaths))
        {
            Best       = PS;
            BestKills  = Kills;
            BestDeaths = Deaths;
            bTie       = false;
        }
        else if (Kills == BestKills && Deaths == BestDeaths)
        {
            bTie = true;
        }
    }

    if (BestKills <= 0 || bTie)
    {
        return nullptr;
    }
    return Best;
}

void ALastFPSMatchGameMode::EndMatch(const FString& Reason, APlayerState* Winner)
{
    if (bMatchEnded)
    {
        return;
    }

    bMatchEnded = true;
    bDropIntroInProgress = false;

    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(DropIntroTimerHandle);
        World->GetTimerManager().ClearTimer(MatchRuleTimerHandle);
    }

    if (ALastFPSMatchGameState* MGS = World ? World->GetGameState<ALastFPSMatchGameState>() : nullptr)
    {
        MGS->Auth_SetDropIntroActive(false);
        MGS->Auth_SetMatchResult(Winner, Reason);
    }

    DebugFlow(
        FString::Printf(
            TEXT("[Match] Deathmatch ended. Winner=%s | Reason: %s"),
            Winner ? *Winner->GetPlayerName() : TEXT("(none/draw)"),
            *Reason),
        FColor::Red);

    if (World)
    {
        World->GetTimerManager().SetTimer(
            ResultDisplayTimerHandle,
            this,
            &ALastFPSMatchGameMode::TravelToLobby,
            FMath::Max(0.1f, MatchResultDisplaySeconds),
            false);
    }
}

void ALastFPSMatchGameMode::TravelToLobby()
{
    if (LobbyMapURL.IsEmpty())
    {
        DebugFlow(TEXT("[Match] LobbyMapURL is empty. Return-to-lobby aborted."), FColor::Red);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        DebugFlow(FString::Printf(TEXT("[Match] Returning to lobby: %s"), *LobbyMapURL), FColor::Green);
        World->ServerTravel(LobbyMapURL);
    }
}
