#include "Game/LastFPSMatchGameMode.h"

#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Character/LastFPSCharacterBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"

void ALastFPSMatchGameMode::BeginPlay()
{
    Super::BeginPlay();

    DebugMatchFlow(TEXT("[Match] MatchGameMode BeginPlay"));
    StartDropIntroPhase();
}

void ALastFPSMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    DebugMatchFlow(FString::Printf(
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
    DebugMatchFlow(
        FString::Printf(TEXT("[Match] Drop intro started (%.1f second(s))."), DropIntroSeconds),
        FColor::Blue);

    if (UWorld* World = GetWorld())
    {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            APawn* Pawn = PC ? PC->GetPawn() : nullptr;
            if (!Pawn)
            {
                continue;
            }

            FVector DropLocation = Pawn->GetActorLocation();
            DropLocation.Z += DropHeightOffset;
            Pawn->SetActorLocation(DropLocation);

            if (UCharacterMovementComponent* MoveComp = Pawn->FindComponentByClass<UCharacterMovementComponent>())
            {
                MoveComp->SetMovementMode(MOVE_Falling);
            }
        }

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
    MatchStartServerTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    DebugMatchFlow(
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

    FString EndReason;
    if (IsMatchEndConditionMet(EndReason))
    {
        EndMatchAndReturnToLobby(EndReason);
    }
}

void ALastFPSMatchGameMode::ScheduleRespawnForDeadPlayers()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

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

            DebugMatchFlow(
                FString::Printf(
                    TEXT("[Match] Respawn scheduled: %s in %.1f second(s)."),
                    *PC->GetName(),
                    RespawnDelaySeconds),
                FColor::Yellow);
        }
    }

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

    APawn* OldPawn = ControllerToRespawn->GetPawn();
    RestartPlayer(ControllerToRespawn);

    if (OldPawn)
    {
        OldPawn->Destroy();
    }

    if (ALastFPSPlayerState* PS = ControllerToRespawn->GetPlayerState<ALastFPSPlayerState>())
    {
        if (ULastFPSAttributeSet* AttrSet = PS->GetAttributeSet())
        {
            AttrSet->SetHealth(AttrSet->GetMaxHealth());
            AttrSet->SetStamina(AttrSet->GetMaxStamina());
        }
    }

    DebugMatchFlow(FString::Printf(TEXT("[Match] Respawned: %s"), *ControllerToRespawn->GetName()), FColor::Cyan);
}

bool ALastFPSMatchGameMode::IsMatchEndConditionMet(FString& OutReason) const
{
    UWorld* World = GetWorld();
    if (!World || !GameState)
    {
        return false;
    }

    const float ElapsedSeconds = World->GetTimeSeconds() - MatchStartServerTimeSeconds;
    if (ElapsedSeconds >= MatchDurationSeconds)
    {
        OutReason = FString::Printf(TEXT("Time limit reached (%d sec)"), MatchDurationSeconds);
        return true;
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
            return true;
        }
    }

    return false;
}

void ALastFPSMatchGameMode::EndMatchAndReturnToLobby(const FString& Reason)
{
    if (bMatchEnded)
    {
        return;
    }

    bMatchEnded = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DropIntroTimerHandle);
        World->GetTimerManager().ClearTimer(MatchRuleTimerHandle);
    }

    DebugMatchFlow(FString::Printf(TEXT("[Match] Deathmatch ended. Reason: %s"), *Reason), FColor::Red);

    if (LobbyMapURL.IsEmpty())
    {
        DebugMatchFlow(TEXT("[Match] LobbyMapURL is empty. Return-to-lobby aborted."), FColor::Red);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        DebugMatchFlow(FString::Printf(TEXT("[Match] Returning to lobby: %s"), *LobbyMapURL), FColor::Green);
        World->ServerTravel(LobbyMapURL);
    }
}

void ALastFPSMatchGameMode::DebugMatchFlow(const FString& Message, FColor Color) const
{
    UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, Color, Message);
    }
}
