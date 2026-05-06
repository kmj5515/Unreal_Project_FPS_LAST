#pragma once

#include "CoreMinimal.h"
#include "Game/LastFPSGameModeBase.h"
#include "LastFPSLobbyGameMode.generated.h"

UCLASS()
class LASTFPS_API ALastFPSLobbyGameMode : public ALastFPSGameModeBase
{
    GENERATED_BODY()

public:
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

protected:
    void TryStartMatchFromLobby();
    void DebugLobbyFlow(const FString& Message, FColor Color = FColor::Green) const;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Lobby")
    int32 LobbyStartPlayerCount = 3;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Lobby")
    FString MatchMapURL;

    bool bLobbyMatchStartTriggered = false;
};
