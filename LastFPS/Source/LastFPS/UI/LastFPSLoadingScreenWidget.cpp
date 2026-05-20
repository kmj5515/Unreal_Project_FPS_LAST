#include "UI/LastFPSLoadingScreenWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void ULastFPSLoadingScreenWidget::SetStatusText(const FText& InText)
{
    if (Text_Status)
    {
        Text_Status->SetText(InText);
    }
}

void ULastFPSLoadingScreenWidget::SetMapNameText(const FText& InText)
{
    if (Text_MapName)
    {
        Text_MapName->SetText(InText);
    }
}

void ULastFPSLoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!PB_Loading || IndeterminateCycleSeconds <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    IndeterminatePhase += InDeltaTime / IndeterminateCycleSeconds;
    if (IndeterminatePhase > 1.0f)
    {
        IndeterminatePhase = FMath::Fmod(IndeterminatePhase, 1.0f);
    }

    const float PingPong = 0.5f - 0.5f * FMath::Cos(IndeterminatePhase * 2.0f * PI);
    PB_Loading->SetPercent(PingPong);
}
