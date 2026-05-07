#pragma once

#include "CoreMinimal.h"
#include "Game/LastFPSGameModeBase.h"
#include "TimerManager.h"
#include "LastFPSMatchGameMode.generated.h"

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

    void StartDropIntroPhase();
    void FinishDropIntroPhase();
    void TickMatchRuleCheck();
    void ScheduleRespawnForDeadPlayers();
    void RespawnController(AController* ControllerToRespawn);
    bool IsMatchEndConditionMet(FString& OutReason) const;
    void EndMatchAndReturnToLobby(const FString& Reason);
    void DebugMatchFlow(const FString& Message, FColor Color = FColor::Green) const;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    int32 MatchDurationSeconds = 180;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    int32 MatchKillLimit = 10;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    float RespawnDelaySeconds = 6.0f;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    float DropIntroSeconds = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    float DropHeightOffset = 1200.0f;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    FString LobbyMapURL;

    bool bDropIntroInProgress = false;
    bool bMatchEnded = false;
    float MatchStartServerTimeSeconds = 0.0f;

    FTimerHandle DropIntroTimerHandle;
    FTimerHandle MatchRuleTimerHandle;
    TMap<TWeakObjectPtr<AController>, float> PendingRespawnControllers;
};
