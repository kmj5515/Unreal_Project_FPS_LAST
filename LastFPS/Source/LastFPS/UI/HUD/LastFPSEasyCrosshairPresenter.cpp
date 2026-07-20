#include "UI/HUD/LastFPSEasyCrosshairPresenter.h"

#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "EasyCrosshairSystem/ecsCrosshairSubsystem.h"
#include "EasyCrosshairSystem/ecsCrosshairWidget.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSEasyCrosshair, Log, All);

bool ULastFPSEasyCrosshairPresenter::ShowCrosshair(
    UWorld& World,
    UecsCrosshairEditorAsset& CrosshairAsset,
    UOverlay* Host,
    const FName InFireAnimationName,
    const float InFireAnimationDuration)
{
    if (ActiveCrosshairAsset.Get() == &CrosshairAsset && CrosshairWidget.IsValid())
    {
        FireAnimationName = InFireAnimationName;
        FireAnimationDuration = InFireAnimationDuration;
        bMissingAnimationWarningLogged = false;
        SetVisible(true);
        return true;
    }

    Shutdown();
    OwningWorld = &World;
    FireAnimationName = InFireAnimationName;
    FireAnimationDuration = InFireAnimationDuration;

    UecsCrosshairSubsystem* CrosshairSubsystem = World.GetSubsystem<UecsCrosshairSubsystem>();
    if (!CrosshairSubsystem)
    {
        UE_LOG(LogLastFPSEasyCrosshair, Error, TEXT("Easy Crosshair Subsystem을 찾지 못했습니다."));
        return false;
    }

    CrosshairSubsystem->SetupCrosshair(&CrosshairAsset);
    UecsCrosshairWidget* CreatedWidget = CrosshairSubsystem->GetCrosshairWidget();
    if (!CreatedWidget)
    {
        UE_LOG(
            LogLastFPSEasyCrosshair,
            Error,
            TEXT("EasyCrosshair 위젯 생성을 실패했습니다. 에셋: '%s'."),
            *GetNameSafe(&CrosshairAsset));
        return false;
    }

    if (Host)
    {
        CreatedWidget->RemoveFromParent();
        if (UOverlaySlot* CrosshairSlot = Host->AddChildToOverlay(CreatedWidget))
        {
            CrosshairSlot->SetHorizontalAlignment(HAlign_Center);
            CrosshairSlot->SetVerticalAlignment(VAlign_Center);
        }
    }

    CrosshairWidget = CreatedWidget;
    ActiveCrosshairAsset = &CrosshairAsset;
    bMissingAnimationWarningLogged = false;
    SetVisible(true);
    return true;
}

void ULastFPSEasyCrosshairPresenter::SetVisible(const bool bVisible)
{
    if (UecsCrosshairWidget* Widget = CrosshairWidget.Get())
    {
        Widget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void ULastFPSEasyCrosshairPresenter::PlayFireAnimation()
{
    UecsCrosshairWidget* Widget = CrosshairWidget.Get();
    UecsCrosshairEditorAsset* CrosshairAsset = ActiveCrosshairAsset.Get();
    if (!Widget || !CrosshairAsset || FireAnimationName.IsNone())
    {
        return;
    }

    const FecsCrosshairAnimation Animation = CrosshairAsset->GetAnimationByName(FireAnimationName);
    if (Animation.AnimationLayers.IsEmpty())
    {
        if (!bMissingAnimationWarningLogged)
        {
            UE_LOG(
                LogLastFPSEasyCrosshair,
                Warning,
                TEXT("EasyCrosshair 에셋 '%s'에 발사 애니메이션 '%s'이 없습니다."),
                *GetNameSafe(CrosshairAsset),
                *FireAnimationName.ToString());
            bMissingAnimationWarningLogged = true;
        }
        return;
    }

    UWorld* World = OwningWorld.Get();
    UecsCrosshairSubsystem* CrosshairSubsystem = World ? World->GetSubsystem<UecsCrosshairSubsystem>() : nullptr;
    if (CrosshairSubsystem && CrosshairSubsystem->GetCrosshairWidget() == Widget)
    {
        CrosshairSubsystem->RunAnimation(FireAnimationName, FireAnimationDuration);
    }
}

void ULastFPSEasyCrosshairPresenter::Shutdown()
{
    UWorld* World = OwningWorld.Get();
    UecsCrosshairSubsystem* CrosshairSubsystem = World ? World->GetSubsystem<UecsCrosshairSubsystem>() : nullptr;
    if (CrosshairSubsystem && CrosshairSubsystem->GetCrosshairWidget() == CrosshairWidget.Get())
    {
        CrosshairSubsystem->RemoveCrosshair();
    }

    OwningWorld.Reset();
    CrosshairWidget.Reset();
    ActiveCrosshairAsset.Reset();
    FireAnimationName = NAME_None;
    FireAnimationDuration = 0.f;
    bMissingAnimationWarningLogged = false;
}
