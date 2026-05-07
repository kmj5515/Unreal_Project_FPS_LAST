#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LastFPSLobbyHUD.generated.h"

class UUserWidget;

UCLASS()
class LASTFPS_API ALastFPSLobbyHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="LobbyHUD")
    TSubclassOf<UUserWidget> LobbyWidgetClass;

private:
    UPROPERTY()
    TObjectPtr<UUserWidget> LobbyWidget;
};

