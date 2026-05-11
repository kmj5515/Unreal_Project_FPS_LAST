#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LastFPSMatchGameState.generated.h"

UCLASS()
class LASTFPS_API ALastFPSMatchGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ALastFPSMatchGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category="LastFPS|Match")
    float GetMatchTimeRemaining() const { return MatchTimeRemaining; }

    /** 서버 전용 — 권한 없으면 무시 */
    void Auth_SetMatchTimeRemaining(float NewSeconds);

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Match")
    float MatchTimeRemaining = 0.f;
};
