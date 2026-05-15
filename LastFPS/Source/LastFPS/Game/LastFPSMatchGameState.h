#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LastFPSMatchGameState.generated.h"

class APlayerState;

DECLARE_MULTICAST_DELEGATE(FOnLastFPSMatchEnded);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLastFPSKillFeed, const FString&, /*Killer*/ const FString& /*Victim*/);

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
    bool IsDropIntroActive() const { return bDropIntroActive; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Match")
    APlayerState* GetWinnerPlayerState() const { return WinnerPlayerState.Get(); }

    UFUNCTION(BlueprintPure, Category="LastFPS|Match")
    const FString& GetEndReason() const { return EndReason; }

    FOnLastFPSMatchEnded OnMatchEnded;
    FOnLastFPSKillFeed OnKillFeed;

    void Auth_SetMatchTimeRemaining(float NewSeconds);
    void Auth_BroadcastKillFeed(class ALastFPSPlayerState* KillerPS, class ALastFPSPlayerState* VictimPS);
    void Auth_SetMatchResult(APlayerState* InWinner, const FString& InReason);
    void Auth_SetDropIntroActive(bool bActive);

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Match")
    float MatchTimeRemaining = 0.f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Match")
    bool bDropIntroActive = false;

    UPROPERTY(ReplicatedUsing=OnRep_MatchEnded, BlueprintReadOnly, Category="LastFPS|Match")
    bool bMatchEnded = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Match")
    TWeakObjectPtr<APlayerState> WinnerPlayerState;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Match")
    FString EndReason;

    UFUNCTION()
    void OnRep_MatchEnded();

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_KillFeed(const FString& KillerName, const FString& VictimName);
};
