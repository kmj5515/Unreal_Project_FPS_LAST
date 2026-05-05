#include "UI/LastFPSHUD.h"
#include "UI/LastFPSHUDWidget.h"
#include "UI/LastFPSScoreboardWidget.h"

void ALastFPSHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = GetOwningPlayerController();

    if (HUDWidgetClass)
    {
        HUDWidget = CreateWidget<ULastFPSHUDWidget>(PC, HUDWidgetClass);
        if (HUDWidget)
            HUDWidget->AddToViewport(0);
    }

    if (ScoreboardWidgetClass)
    {
        ScoreboardWidget = CreateWidget<ULastFPSScoreboardWidget>(PC, ScoreboardWidgetClass);
        if (ScoreboardWidget)
        {
            ScoreboardWidget->AddToViewport(1);
            ScoreboardWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
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
    ScoreboardWidget->SetVisibility(ESlateVisibility::Visible);
}

void ALastFPSHUD::HideScoreboard()
{
    if (ScoreboardWidget)
        ScoreboardWidget->SetVisibility(ESlateVisibility::Collapsed);
}
