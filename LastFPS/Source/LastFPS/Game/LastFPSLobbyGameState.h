#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LastFPSLobbyGameState.generated.h"

UCLASS()
class LASTFPS_API ALastFPSLobbyGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Lobby")
    int32 LobbyStartPlayerCount = 3;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Lobby")
    int32 RemainingCharacterSelectSeconds = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Lobby")
    bool bCharacterSelectInProgress = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Lobby")
    bool bTeamIntroInProgress = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Lobby")
    bool bTravelTriggered = false;
};

