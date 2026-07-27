#include "UI/Loading/LastFPSLoadingScreenWidget.h"

#include "Game/Loading/LastFPSLoadingProcessSubsystem.h"
#include "Game/LastFPSGameInstance.h"
#include "Data/Tables/LastFPSLoadingTipData.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSLoadingScreen, Log, All);

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

    if (PB_Loading)
    {
        PB_Loading->SetPercent(0.0f);
        PB_Loading->SetIsMarquee(false);
    }
    else
    {
        UE_LOG(LogLastFPSLoadingScreen, Warning,
            TEXT("WBP_LoadingScreen에 이름이 PB_Loading인 Progress Bar가 없어 진행 표시를 바인딩하지 못했습니다."));
    }

    if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
    {
        TravelPresentationChangedHandle = GI->OnTravelPresentationChanged.AddUObject(
            this, &ULastFPSLoadingScreenWidget::HandleTravelPresentationChanged);
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (ULastFPSLoadingProcessSubsystem* LoadingProcesses =
            GameInstance->GetSubsystem<ULastFPSLoadingProcessSubsystem>())
        {
            LoadingProgressChangedHandle = LoadingProcesses->OnLoadingProgressChanged.AddUObject(
                this, &ULastFPSLoadingScreenWidget::HandleLoadingProgressChanged);

            if (LoadingProcesses->IsLoadingActive())
            {
                HandleLoadingProgressChanged(LoadingProcesses->GetOverallProgress(), FGameplayTag());
            }
        }
    }
}

void ULastFPSLoadingScreenWidget::NativeDestruct()
{
    if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
    {
        GI->OnTravelPresentationChanged.Remove(TravelPresentationChangedHandle);
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (ULastFPSLoadingProcessSubsystem* LoadingProcesses =
            GameInstance->GetSubsystem<ULastFPSLoadingProcessSubsystem>())
        {
            LoadingProcesses->OnLoadingProgressChanged.Remove(LoadingProgressChangedHandle);
        }
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

void ULastFPSLoadingScreenWidget::HandleLoadingProgressChanged(
    const float OverallProgress,
    FGameplayTag /*변경된 프로세스 태그*/)
{
    if (!PB_Loading)
    {
        return;
    }

    PB_Loading->SetIsMarquee(false);
    PB_Loading->SetPercent(FMath::Clamp(OverallProgress, 0.0f, 1.0f));
}
