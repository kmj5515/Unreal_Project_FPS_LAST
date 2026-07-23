#include "UI/HUD/Presenters/LastFPSGrapplingReticlePresenter.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSGrapplingReticle, Log, All);

void ULastFPSGrapplingReticlePresenter::Initialize(
    UImage* InDotImage,
    UWidgetTree* InWidgetTree,
    const FLastFPSGrapplingReticleConfig& InConfig)
{
    DotImage = InDotImage;
    WidgetTree = InWidgetTree;
    Config = InConfig;
    bInitialized = false;
}

void ULastFPSGrapplingReticlePresenter::Reset()
{
    bInitialized = false;
}

void ULastFPSGrapplingReticlePresenter::EnsureDot(UOverlay* Host)
{
    if (Host)
    {
        HostOverlay = Host;
    }

    if (!DotImage)
    {
        UWidgetTree* Tree = WidgetTree.Get();
        UOverlay* ResolvedHost = HostOverlay.Get();
        if (!ResolvedHost || !Tree)
        {
            return;
        }

        UImage* RuntimeDot = Tree->ConstructWidget<UImage>(
            UImage::StaticClass(),
            TEXT("RuntimeGrapplingDotImage"));
        if (!RuntimeDot)
        {
            UE_LOG(LogLastFPSGrapplingReticle, Error, TEXT("그래플링 조준점 생성을 실패했습니다."));
            return;
        }

        const float DotSize = FMath::Max(Config.DotSize, 1.f);
        const FSlateRoundedBoxBrush DotBrush(FLinearColor::White, FVector2f(DotSize, DotSize));
        RuntimeDot->SetBrush(DotBrush);

        if (UOverlaySlot* DotSlot = ResolvedHost->AddChildToOverlay(RuntimeDot))
        {
            DotSlot->SetHorizontalAlignment(HAlign_Center);
            DotSlot->SetVerticalAlignment(VAlign_Center);
        }

        DotImage = RuntimeDot;
    }

    if (bInitialized)
    {
        return;
    }

    CurrentScale = FMath::Max(Config.IdleScale, 0.01f);
    TargetScale = CurrentScale;
    DotImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    DotImage->SetRenderScale(FVector2D(CurrentScale, CurrentScale));
    DotImage->SetColorAndOpacity(Config.IdleColor);
    DotImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    bInitialized = true;
}

void ULastFPSGrapplingReticlePresenter::SetAvailability(bool bTargetAvailable)
{
    EnsureDot(nullptr);
    TargetScale = FMath::Max(
        bTargetAvailable ? Config.AvailableScale : Config.IdleScale,
        0.01f);

    if (DotImage)
    {
        DotImage->SetColorAndOpacity(bTargetAvailable ? Config.AvailableColor : Config.IdleColor);
    }
}

void ULastFPSGrapplingReticlePresenter::Tick(float DeltaTime)
{
    if (!DotImage)
    {
        return;
    }

    if (Config.ScaleInterpSpeed <= KINDA_SMALL_NUMBER)
    {
        CurrentScale = TargetScale;
    }
    else
    {
        CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, DeltaTime, Config.ScaleInterpSpeed);
    }

    if (FMath::IsNearlyEqual(CurrentScale, TargetScale, 0.001f))
    {
        CurrentScale = TargetScale;
    }

    DotImage->SetRenderScale(FVector2D(CurrentScale, CurrentScale));
}
