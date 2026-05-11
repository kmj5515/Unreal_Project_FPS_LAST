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

    void ApplyDropIntroToController(APlayerController* PlayerController) const;
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
    /** 매치 종료 처리: 결과를 GameState에 기록 → 결과 표시 시간 후 트래블 */
    void EndMatch(const FString& Reason, class APlayerState* Winner);
    void TravelToLobby();
    /** 시간 만료용 — 최다 킬(동률 시 데스 적은 사람) 선정. 동률·후보 없음 시 nullptr */
    class APlayerState* DetermineLeadingPlayer() const;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    int32 MatchDurationSeconds = 180;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    int32 MatchKillLimit = 3;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    float RespawnDelaySeconds = 6.0f;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    float DropIntroSeconds = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Match")
    float DropHeightOffset = 1200.0f;

    /** 매치 종료 후 결과 화면을 표시하는 시간(초). 이 시간 후 ServerTravel(LobbyMapURL) */
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
};
