#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Game/LastFPSGameModeBase.h"
#include "LastFPSMatchGameState.generated.h"

UCLASS()
class LASTFPS_API ALastFPSMatchGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ALastFPSMatchGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Match")
    int32 GetTeamScore(ELastFPSTeam Team) const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Match")
    const TArray<int32>& GetAllTeamScores() const { return TeamScores; }

    /** 서버 전용 — 권한 없으면 무시 */
    void Auth_AddTeamScore(ELastFPSTeam Team, int32 Delta);

protected:
    // 인덱스 = ELastFPSTeam 정수값. 크기는 ELastFPSTeam의 유효 팀 수(4)
    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Match")
    TArray<int32> TeamScores;
};
