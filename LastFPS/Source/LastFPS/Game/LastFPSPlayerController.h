#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LastFPSPlayerController.generated.h"

UCLASS()
class LASTFPS_API ALastFPSPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    void SetLobbyReady(bool bReady);

    UFUNCTION(BlueprintPure, Category="LastFPS|Lobby")
    bool IsLobbyReady() const { return bLobbyReady; }

protected:
    UFUNCTION(Server, Reliable)
    void ServerSetLobbyReady(bool bReady);

    UPROPERTY(BlueprintReadOnly, Category="LastFPS|Lobby")
    bool bLobbyReady = false;
};
