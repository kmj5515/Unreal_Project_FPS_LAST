#include "UI/LastFPSLobbyHUD.h"

#include "UI/LastFPSLobbyWidget.h"
#include "UI/LastFPSUITags.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "PrimaryGameLayout.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSLobbyHUD, Log, All);

void ALastFPSLobbyHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !PC->IsLocalController())
    {
        return;
    }

    TryPushLobbyWidget();
}

void ALastFPSLobbyHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UIPushRetryTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void ALastFPSLobbyHUD::TryPushLobbyWidget()
{
    if (bLobbyWidgetPushed || !LobbyWidgetClass)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayerController();
    if (!PC)
    {
        return;
    }

    UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(PC);
    if (!RootLayout)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                UIPushRetryTimerHandle,
                this,
                &ALastFPSLobbyHUD::RetryPushLobbyWidget,
                0.1f,
                true);
        }
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UIPushRetryTimerHandle);
    }

    LobbyWidget = RootLayout->PushWidgetToLayerStack<ULastFPSLobbyWidget>(
        LastFPSUITags::Layer_Menu(),
        LobbyWidgetClass);

    if (LobbyWidget)
    {
        bLobbyWidgetPushed = true;
        UE_LOG(LogLastFPSLobbyHUD, Log,
            TEXT("Lobby widget pushed to UI.Layer.Menu. Class=%s"),
            *LobbyWidgetClass->GetName());
    }
    else
    {
        UE_LOG(LogLastFPSLobbyHUD, Error, TEXT("Failed to push lobby widget to layout"));
    }
}

void ALastFPSLobbyHUD::RetryPushLobbyWidget()
{
    TryPushLobbyWidget();
}
