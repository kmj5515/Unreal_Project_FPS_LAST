#pragma once

#include "CoreMinimal.h"
#include "Game/LastFPSGameModeBase.h"
#include "LastFPSMatchGameMode.generated.h"

UCLASS()
class LASTFPS_API ALastFPSMatchGameMode : public ALastFPSGameModeBase
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
};
