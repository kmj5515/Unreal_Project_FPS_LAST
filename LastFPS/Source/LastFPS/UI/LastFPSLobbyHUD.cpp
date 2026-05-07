#include "UI/LastFPSLobbyHUD.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void ALastFPSLobbyHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = GetOwningPlayerController();
    if (!PC)
    {
        return;
    }

    if (LobbyWidgetClass)
    {
        LobbyWidget = CreateWidget<UUserWidget>(PC, LobbyWidgetClass);
        if (LobbyWidget)
        {
            LobbyWidget->AddToViewport(0);
        }
    }
}

