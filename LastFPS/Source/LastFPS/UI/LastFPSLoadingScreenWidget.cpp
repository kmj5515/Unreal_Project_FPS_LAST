#include "UI/LastFPSLoadingScreenWidget.h"

#include "Game/LastFPSGameInstance.h"
#include "UI/LastFPSLoadingScreenSet.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

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
    ApplyRandomTip();

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

void ULastFPSLoadingScreenWidget::ApplyRandomTip()
{
    if (!LoadingSet)
    {
        return;
    }

    const FLastFPSLoadingTip* Entry = LoadingSet->PickRandomEntry();
    if (!Entry)
    {
        return;
    }

    // 로딩 화면 위라 동기 로드의 히치는 사실상 보이지 않는다 — 선택된 1장만 즉시 올린다.
    UTexture2D* Texture = Entry->Image.LoadSynchronous();

    if (Img_Tip)
    {
        if (Texture)
        {
            Img_Tip->SetBrushFromTexture(Texture);
            Img_Tip->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            Img_Tip->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    if (Text_TipTitle)
    {
        Text_TipTitle->SetText(Entry->TipTitle);
    }
    if (Text_TipBody)
    {
        Text_TipBody->SetText(Entry->TipBody);
    }

    OnLoadingTipSelected(Texture, Entry->TipTitle, Entry->TipBody);
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
