#include "UI/LastFPSHUD.h"
#include "UI/LastFPSHUDWidget.h"

void ALastFPSHUD::BeginPlay()
{
    Super::BeginPlay();

    if (!HUDWidgetClass) return;

    HUDWidget = CreateWidget<ULastFPSHUDWidget>(GetOwningPlayerController(), HUDWidgetClass);
    if (HUDWidget)
        HUDWidget->AddToViewport();
}
