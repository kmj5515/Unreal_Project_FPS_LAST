#include "UI/HUD/Presenters/LastFPSReloadPresenter.h"

#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULastFPSReloadPresenter::Initialize(UImage* InReloadImage,UOverlay* Overlay, FName InProgressParameterName)
{
    ReloadOverlay = Overlay;
    ReloadImage = InReloadImage;
    ProgressParameterName = InProgressParameterName;
    bReloadInProgress = false;
    ReloadElapsedSeconds = 0.f;
    ReloadTotalSeconds = 0.f;
}

void ULastFPSReloadPresenter::SetReloadStarted(float ReloadDuration)
{
    // 소요 시간이 유효하지 않으면 즉시 완료로 간주해 진행 표시를 띄우지 않는다.
    ReloadTotalSeconds = FMath::Max(ReloadDuration, 0.f);
    ReloadElapsedSeconds = 0.f;
    bReloadInProgress = ReloadTotalSeconds > KINDA_SMALL_NUMBER;

    if (!bReloadInProgress)
    {
        return;
    }

    UpdateProgress(0.f);
    SetVisible(true);
}

void ULastFPSReloadPresenter::SetReloadFinished(bool bCompleted)
{
    bReloadInProgress = false;
    ReloadElapsedSeconds = 0.f;

    // 정상 완료는 가득 찬 상태로, 취소는 빈 상태로 마감한 뒤 숨긴다.
    UpdateProgress(bCompleted ? 1.f : 0.f);
    SetVisible(false);
}

void ULastFPSReloadPresenter::Tick(float DeltaTime)
{
    if (!bReloadInProgress)
    {
        return;
    }

    ReloadElapsedSeconds += DeltaTime;
    const float Progress = ReloadTotalSeconds > KINDA_SMALL_NUMBER
        ? FMath::Clamp(ReloadElapsedSeconds / ReloadTotalSeconds, 0.f, 1.f)
        : 1.f;

    UpdateProgress(Progress);

    // 실제 표시 종료는 어빌리티의 완료/취소 알림(HandleReloadFinished)에서 처리한다.
    // 여기서는 종료 알림이 도착하기 전까지 시각적으로 100%에서 멈춰 대기한다.
}

void ULastFPSReloadPresenter::SetVisible(bool bVisible)
{
    if (ReloadOverlay)
    {
        ReloadOverlay->SetVisibility(bVisible
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed);
    }
    
    if (ReloadImage)
    {
        ReloadImage->SetVisibility(bVisible
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed);
    }
}

void ULastFPSReloadPresenter::UpdateProgress(float Progress)
{
    if (ReloadImage)
    {
        if (UMaterialInstanceDynamic* Material = ReloadImage->GetDynamicMaterial())
        {
            Material->SetScalarParameterValue(ProgressParameterName, Progress);
        }
    }
}
