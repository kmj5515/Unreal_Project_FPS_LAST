#include "UI/HUD/LastFPSEasyCrosshairPresenter.h"

#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "UI/Crosshair/LastFPSCrosshairSpreadBehavior.h"
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
    if (SourceCrosshairAsset.Get() == &CrosshairAsset && CrosshairWidget.IsValid())
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

    // 공유 에셋 대신 런타임 사본을 사용 — 에셋에 인스턴스된 Behavior가 위젯 포인터 등
    // 런타임 상태를 원본에 남기는 것을 차단한다(에디터 Undo 버퍼 경유 GC 누수 방지).
    // 인스턴스된 하위 객체(Behavior·애니메이션 레이어)는 함께 복제되고 텍스처는 공유된다.
    UecsCrosshairEditorAsset* RuntimeAsset =
        DuplicateObject<UecsCrosshairEditorAsset>(&CrosshairAsset, GetTransientPackage());
    if (!RuntimeAsset)
    {
        UE_LOG(
            LogLastFPSEasyCrosshair,
            Error,
            TEXT("EasyCrosshair 에셋 '%s'의 런타임 사본 생성을 실패했습니다."),
            *GetNameSafe(&CrosshairAsset));
        return false;
    }

    CrosshairSubsystem->SetupCrosshair(RuntimeAsset);
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
    SourceCrosshairAsset = &CrosshairAsset;
    RuntimeCrosshairAsset = RuntimeAsset;
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
    UecsCrosshairEditorAsset* CrosshairAsset = RuntimeCrosshairAsset;
    if (!Widget || !CrosshairAsset)
    {
        return;
    }

    // 연사 누적 벌어짐은 애니메이션(고정 커브)으로 표현할 수 없어 SpreadBehavior의
    // 누적값으로 처리한다. 애니메이션 유무와 무관하게 발사마다 통지한다.
    for (UecsCrosshairBehavior* Behavior : CrosshairAsset->Behaviors)
    {
        if (ULastFPSCrosshairSpreadBehavior* SpreadBehavior = Cast<ULastFPSCrosshairSpreadBehavior>(Behavior))
        {
            SpreadBehavior->NotifyWeaponFired();
        }
    }

    if (FireAnimationName.IsNone())
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
    SourceCrosshairAsset.Reset();
    RuntimeCrosshairAsset = nullptr;
    FireAnimationName = NAME_None;
    FireAnimationDuration = 0.f;
    bMissingAnimationWarningLogged = false;
}
