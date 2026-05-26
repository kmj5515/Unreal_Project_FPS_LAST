#include "UI/LastFPSHUD.h"
#include "UI/LastFPSHUDWidget.h"
#include "UI/LastFPSScoreboardWidget.h"
#include "UI/LastFPSUITags.h"
#include "Game/LastFPSMatchGameState.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "PrimaryGameLayout.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSHUD, Log, All);

void ALastFPSHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !PC->IsLocalController())
    {
        return;
    }

    TryPushWidgetsToUILayout();
    TryBindMatchGameState();
}

void ALastFPSHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
        World->GetTimerManager().ClearTimer(UIPushRetryTimerHandle);
    }

    if (ALastFPSMatchGameState* MGS = BoundMatchGameState.Get())
    {
        MGS->OnMatchEnded.Remove(MatchEndedHandle);
    }
    BoundMatchGameState.Reset();
    MatchEndedHandle.Reset();

    Super::EndPlay(EndPlayReason);
}

void ALastFPSHUD::TryPushWidgetsToUILayout()
{
    if (bWidgetsPushedToLayout)
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
                &ALastFPSHUD::RetryPushWidgetsToUILayout,
                0.1f,
                true);
        }
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UIPushRetryTimerHandle);
    }

    if (HUDWidgetClass && !HUDWidget)
    {
        HUDWidget = RootLayout->PushWidgetToLayerStack<ULastFPSHUDWidget>(
            LastFPSUITags::Layer_Game(),
            HUDWidgetClass);
        if (HUDWidget)
        {
            UE_LOG(
                LogLastFPSHUD,
                Log,
                TEXT("HUD pushed to UI.Layer.Game. Class=%s"),
                *HUDWidgetClass->GetName());
        }
        else
        {
            UE_LOG(LogLastFPSHUD, Error, TEXT("Failed to push HUD widget to layout"));
        }
    }

    if (ScoreboardWidgetClass && !ScoreboardWidget)
    {
        ScoreboardWidget = RootLayout->PushWidgetToLayerStack<ULastFPSScoreboardWidget>(
            LastFPSUITags::Layer_GameMenu(),
            ScoreboardWidgetClass);
        if (ScoreboardWidget)
        {
            ScoreboardWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    bWidgetsPushedToLayout = (HUDWidget != nullptr);
}

void ALastFPSHUD::RetryPushWidgetsToUILayout()
{
    TryPushWidgetsToUILayout();
}

void ALastFPSHUD::TryBindMatchGameState()
{
    UWorld* World = GetWorld();
    if (!World) return;

    ALastFPSMatchGameState* MGS = World->GetGameState<ALastFPSMatchGameState>();
    if (!MGS)
    {
        World->GetTimerManager().SetTimer(
            BindRetryTimerHandle, this, &ALastFPSHUD::RetryBindMatchGameState, 0.1f, true);
        return;
    }

    BoundMatchGameState = MGS;
    MatchEndedHandle    = MGS->OnMatchEnded.AddUObject(this, &ALastFPSHUD::HandleMatchEnded);

    if (MGS->IsMatchEnded())
    {
        HandleMatchEnded();
    }
}

void ALastFPSHUD::RetryBindMatchGameState()
{
    if (BoundMatchGameState.IsValid()) return;

    if (UWorld* World = GetWorld())
    {
        if (ALastFPSMatchGameState* MGS = World->GetGameState<ALastFPSMatchGameState>())
        {
            World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
            BoundMatchGameState = MGS;
            MatchEndedHandle    = MGS->OnMatchEnded.AddUObject(this, &ALastFPSHUD::HandleMatchEnded);
            if (MGS->IsMatchEnded())
            {
                HandleMatchEnded();
            }
        }
    }
}

void ALastFPSHUD::HandleMatchEnded()
{
    bMatchEnded = true;

    ShowScoreboard();

    APlayerController* PC = GetOwningPlayerController();
    ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
    if (UEnhancedInputLocalPlayerSubsystem* Sub =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
    {
        Sub->ClearAllMappings();
    }
}

void ALastFPSHUD::ShowHitMarker()
{
    if (HUDWidget)
        HUDWidget->ShowHitMarker();
}

void ALastFPSHUD::ShowScoreboard()
{
    if (!ScoreboardWidget) return;
    ScoreboardWidget->RefreshScoreboard();
    ScoreboardWidget->StartAutoRefresh();
    ScoreboardWidget->SetVisibility(ESlateVisibility::Visible);
}

void ALastFPSHUD::HideScoreboard()
{
    if (bMatchEnded) return;
    if (!ScoreboardWidget) return;
    ScoreboardWidget->StopAutoRefresh();
    ScoreboardWidget->SetVisibility(ESlateVisibility::Collapsed);
}
