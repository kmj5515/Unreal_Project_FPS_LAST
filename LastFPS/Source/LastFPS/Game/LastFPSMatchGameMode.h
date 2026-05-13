#pragma once

#include "CoreMinimal.h"
#include "Game/LastFPSGameModeBase.h"
#include "TimerManager.h"
#include "LastFPSMatchGameMode.generated.h"

class APlayerStart;

UCLASS()
class LASTFPS_API ALastFPSMatchGameMode : public ALastFPSGameModeBase
{
    GENERATED_BODY()

public:
    ALastFPSMatchGameMode();
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

protected:
    virtual void BeginPlay() override;

    AActor* ChoosePlayerStart_Implementation(AController* Player) override;

    void StartDropIntroPhase();
    void FinishDropIntroPhase();
    void TickMatchRuleCheck();
    void ScheduleRespawnForDeadPlayers();
    void UpdateRespawnSchedule(UWorld* World);
    void ProcessReadyRespawns(UWorld* World);
    void RespawnController(AController* ControllerToRespawn);
    bool IsMatchEndConditionMet(FString& OutReason, class APlayerState*& OutWinner) const;
    bool CheckTimeLimit(FString& OutReason, class APlayerState*& OutWinner) const;
    bool CheckKillLimit(FString& OutReason, class APlayerState*& OutWinner) const;
    void EndMatch(const FString& Reason, class APlayerState* Winner);
    void TravelToLobby();
    class APlayerState* DetermineLeadingPlayer() const;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    int32 MatchDurationSeconds = 180;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    int32 MatchKillLimit = 3;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    float RespawnDelaySeconds = 6.0f;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    float DropIntroSeconds = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match", meta=(ClampMin="0.0"))
    float MatchResultDisplaySeconds = 8.0f;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    FString LobbyMapURL;

    bool bDropIntroInProgress = false;
    bool bMatchEnded = false;
    float MatchStartServerTimeSeconds = 0.0f;

    FTimerHandle DropIntroTimerHandle;
    FTimerHandle MatchRuleTimerHandle;
    FTimerHandle ResultDisplayTimerHandle;
    TMap<TWeakObjectPtr<AController>, float> PendingRespawnControllers;

    TArray<TWeakObjectPtr<APlayerStart>> MatchPlayerStartDeck;

    void RefillMatchPlayerStartDeck(UWorld* World);
};
