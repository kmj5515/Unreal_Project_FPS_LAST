#include "UI/LastFPSLoadingScreenWidget.h"

#include "Game/LastFPSGameInstance.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

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

void ULastFPSLoadingScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshFromGameInstance();

    if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
    {
        TravelPresentationChangedHandle = GI->OnTravelPresentationChanged.AddUObject(
            this, &ULastFPSLoadingScreenWidget::HandleTravelPresentationChanged);
    }
}

void ULastFPSLoadingScreenWidget::NativeDestruct()
{
    if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
    {
        GI->OnTravelPresentationChanged.Remove(TravelPresentationChangedHandle);
    }
    Super::NativeDestruct();
}

void ULastFPSLoadingScreenWidget::RefreshFromGameInstance()
{
    if (const ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
    {
        const FText Status = GI->GetPendingTravelStatusText();
        const FText MapName = GI->GetPendingTravelMapNameText();

        if (!Status.IsEmpty())
        {
            SetStatusText(Status);
        }
        if (!MapName.IsEmpty())
        {
            SetMapNameText(MapName);
        }

        OnLoadingScreenUpdated(Status, MapName);
    }
}

void ULastFPSLoadingScreenWidget::HandleTravelPresentationChanged(const FText& StatusText, const FText& MapNameText)
{
    SetStatusText(StatusText);
    SetMapNameText(MapNameText);
    OnLoadingScreenUpdated(StatusText, MapNameText);
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
