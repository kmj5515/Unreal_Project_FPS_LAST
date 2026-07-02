#include "UI/Loading/LastFPSLoadingScreenWidget.h"

#include "Game/LastFPSGameInstance.h"
#include "Data/Tables/LastFPSLoadingTipData.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
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
    // 프리로드(GameInstance)와 동일한 DT를 단일 소스로 사용 → 표시 텍스처가 항상 상주 캐시와 일치.
    // GI를 못 얻으면 위젯 자체 TipTable로 폴백.
    UDataTable* Table = nullptr;
    if (ULastFPSGameInstance* GI = Cast<ULastFPSGameInstance>(GetGameInstance()))
    {
        Table = GI->GetLoadingTipTable();
    }
    if (!Table)
    {
        Table = TipTable.LoadSynchronous();
    }
    if (!Table)
    {
        return;
    }

    FLastFPSLoadingTipData Tip;
    if (!ULastFPSLoadingTipLibrary::GetRandomLoadingTip(Table, Tip))
    {
        return;
    }

    // 로딩 화면 위라 동기 로드의 히치는 사실상 보이지 않는다 — 선택된 1장만 즉시 올린다.
    UTexture2D* Texture = Tip.Image.LoadSynchronous();

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
        Text_TipTitle->SetText(Tip.TipTitle);
    }
    if (Text_TipBody)
    {
        Text_TipBody->SetText(Tip.Tip);
    }

    OnLoadingTipSelected(Texture, Tip.TipTitle, Tip.Tip);
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
