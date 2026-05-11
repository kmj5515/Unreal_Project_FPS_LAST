#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LastFPSMatchGameState.generated.h"

class APlayerState;

/** 매치 결과가 갱신되어 모든 클라이언트에서 결과 화면을 띄울 시점에 발동 (서버/클라 모두에서 호출됨) */
DECLARE_MULTICAST_DELEGATE(FOnLastFPSMatchEnded);

UCLASS()
class LASTFPS_API ALastFPSMatchGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ALastFPSMatchGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category="LastFPS|Match")
    float GetMatchTimeRemaining() const { return MatchTimeRemaining; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Match")
    bool IsMatchEnded() const { return bMatchEnded; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Match")
    APlayerState* GetWinnerPlayerState() const { return WinnerPlayerState.Get(); }

    UFUNCTION(BlueprintPure, Category="LastFPS|Match")
    const FString& GetEndReason() const { return EndReason; }

    /** HUD가 이 델리게이트에 바인딩하여 결과 화면 표시 */
    FOnLastFPSMatchEnded OnMatchEnded;

    /** 서버 전용 — 권한 없으면 무시 */
    void Auth_SetMatchTimeRemaining(float NewSeconds);
    void Auth_SetMatchResult(APlayerState* InWinner, const FString& InReason);

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Match")
    float MatchTimeRemaining = 0.f;

    UPROPERTY(ReplicatedUsing=OnRep_MatchEnded, BlueprintReadOnly, Category="LastFPS|Match")
    bool bMatchEnded = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Match")
    TWeakObjectPtr<APlayerState> WinnerPlayerState;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Match")
    FString EndReason;

    UFUNCTION()
    void OnRep_MatchEnded();
};
