#include "UI/HUD/Presenters/LastFPSCombatFeedbackPresenter.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/HUD/LastFPSDamageDirectionIndicatorWidget.h"
#include "UI/LastFPSDamageNumberWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSCombatFeedback, Log, All);

void ULastFPSCombatFeedbackPresenter::Initialize(
    UImage* InHitMarkerImage,
    UOverlay* InDamageDirectionLayer,
    const FLastFPSCombatFeedbackConfig& InConfig)
{
    HitMarkerImage = InHitMarkerImage;
    DamageDirectionIndicatorLayer = InDamageDirectionLayer;
    Config = InConfig;

    if (HitMarkerImage)
    {
        InitializeHitMarkerMaterial();
        SetHitMarkerSpread(0.f);
        HitMarkerImage->SetVisibility(ESlateVisibility::Collapsed);
    }

    bHitMarkerSpreadAnimating = false;
    HitMarkerSpreadElapsed = 0.f;
}

void ULastFPSCombatFeedbackPresenter::Shutdown()
{
    ClearDamageDirectionIndicators();
}

void ULastFPSCombatFeedbackPresenter::InitializeHitMarkerMaterial()
{
    if (!HitMarkerImage || HitMarkerMaterial.IsValid())
    {
        return;
    }

    HitMarkerMaterial = HitMarkerImage->GetDynamicMaterial();
}

void ULastFPSCombatFeedbackPresenter::SetHitMarkerSpread(float Spread)
{
    InitializeHitMarkerMaterial();

    if (UMaterialInstanceDynamic* Material = HitMarkerMaterial.Get())
    {
        Material->SetScalarParameterValue(Config.HitMarkerSpreadParameterName, Spread);
    }
}

void ULastFPSCombatFeedbackPresenter::ShowHitMarker()
{
    if (!HitMarkerImage)
    {
        return;
    }

    HitMarkerSpreadElapsed = 0.f;
    bHitMarkerSpreadAnimating = true;
    SetHitMarkerSpread(0.f);

    HitMarkerImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULastFPSCombatFeedbackPresenter::HideHitMarker()
{
    if (HitMarkerImage)
    {
        HitMarkerImage->SetVisibility(ESlateVisibility::Collapsed);
    }

    bHitMarkerSpreadAnimating = false;
    HitMarkerSpreadElapsed = 0.f;
    SetHitMarkerSpread(0.f);
}

void ULastFPSCombatFeedbackPresenter::TickHitMarkerSpread(float DeltaTime)
{
    if (!bHitMarkerSpreadAnimating)
    {
        return;
    }

    HitMarkerSpreadElapsed += DeltaTime;

    const float Alpha = FMath::Clamp(
        HitMarkerSpreadElapsed / FMath::Max(Config.HitMarkerSpreadExpandDuration, KINDA_SMALL_NUMBER),
        0.f,
        1.f);

    SetHitMarkerSpread(FMath::Lerp(0.f, Config.HitMarkerMaxSpread, Alpha));

    if (Alpha >= 1.f)
    {
        HideHitMarker();
    }
}

void ULastFPSCombatFeedbackPresenter::ShowDamageDirection(
    APlayerController* OwningPlayer,
    const FVector& DamageSourceDirection)
{
    if (!DamageDirectionIndicatorLayer || !Config.DamageDirectionIndicatorWidgetClass || !OwningPlayer)
    {
        if (!bDamageDirectionConfigurationWarningLogged)
        {
            UE_LOG(
                LogLastFPSCombatFeedback,
                Warning,
                TEXT("공격 방향 위젯을 생성하지 못했습니다: Layer=%s, WidgetClass=%s, OwningPlayer=%s"),
                *GetNameSafe(DamageDirectionIndicatorLayer.Get()),
                *GetNameSafe(Config.DamageDirectionIndicatorWidgetClass.Get()),
                *GetNameSafe(OwningPlayer));
            bDamageDirectionConfigurationWarningLogged = true;
        }
        return;
    }

    const int32 IndicatorLimit = FMath::Max(Config.MaxDamageDirectionIndicators, 1);
    while (ActiveDamageDirectionIndicators.Num() >= IndicatorLimit)
    {
        if (ULastFPSDamageDirectionIndicatorWidget* OldestIndicator = ActiveDamageDirectionIndicators[0])
        {
            OldestIndicator->RemoveFromParent();
        }
        ActiveDamageDirectionIndicators.RemoveAt(0);
    }

    ULastFPSDamageDirectionIndicatorWidget* Indicator = CreateWidget<ULastFPSDamageDirectionIndicatorWidget>(
        OwningPlayer,
        Config.DamageDirectionIndicatorWidgetClass);
    if (!Indicator)
    {
        UE_LOG(
            LogLastFPSCombatFeedback,
            Warning,
            TEXT("공격 방향 위젯 클래스 '%s'의 인스턴스를 만들지 못했습니다."),
            *GetNameSafe(Config.DamageDirectionIndicatorWidgetClass.Get()));
        return;
    }

    UOverlaySlot* IndicatorSlot = DamageDirectionIndicatorLayer->AddChildToOverlay(Indicator);
    if (!IndicatorSlot)
    {
        UE_LOG(
            LogLastFPSCombatFeedback,
            Warning,
            TEXT("공격 방향 레이어 '%s'에 위젯을 추가하지 못했습니다."),
            *GetNameSafe(DamageDirectionIndicatorLayer.Get()));
        Indicator->RemoveFromParent();
        return;
    }

    IndicatorSlot->SetHorizontalAlignment(HAlign_Fill);
    IndicatorSlot->SetVerticalAlignment(VAlign_Fill);
    if (!Indicator->InitializeDamageDirection(DamageSourceDirection))
    {
        Indicator->RemoveFromParent();
        return;
    }

    Indicator->AdvanceIndicator(0.f, OwningPlayer->GetControlRotation());
    ActiveDamageDirectionIndicators.Add(Indicator);
}

void ULastFPSCombatFeedbackPresenter::TickDamageDirectionIndicators(
    APlayerController* OwningPlayer,
    float DeltaTime)
{
    if (ActiveDamageDirectionIndicators.IsEmpty())
    {
        return;
    }

    if (!OwningPlayer)
    {
        ClearDamageDirectionIndicators();
        return;
    }

    const FRotator ViewRotation = OwningPlayer->GetControlRotation();
    for (int32 Index = ActiveDamageDirectionIndicators.Num() - 1; Index >= 0; --Index)
    {
        ULastFPSDamageDirectionIndicatorWidget* Indicator = ActiveDamageDirectionIndicators[Index];
        if (!IsValid(Indicator) || !Indicator->AdvanceIndicator(DeltaTime, ViewRotation))
        {
            if (IsValid(Indicator))
            {
                Indicator->RemoveFromParent();
            }
            ActiveDamageDirectionIndicators.RemoveAt(Index);
        }
    }
}

void ULastFPSCombatFeedbackPresenter::ClearDamageDirectionIndicators()
{
    for (ULastFPSDamageDirectionIndicatorWidget* Indicator : ActiveDamageDirectionIndicators)
    {
        if (IsValid(Indicator))
        {
            Indicator->RemoveFromParent();
        }
    }
    ActiveDamageDirectionIndicators.Reset();
}

void ULastFPSCombatFeedbackPresenter::SpawnDamageNumber(
    APlayerController* OwningPlayer,
    float DamageAmount,
    float TotalDamageDealt,
    const FVector& DamageWorldLocation,
    AActor* DamageTargetActor,
    bool bCriticalHit)
{
    if (!OwningPlayer || !Config.DamageNumberWidgetClass || DamageAmount <= 0.f)
    {
        return;
    }

    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    OwningPlayer->GetViewportSize(ViewportSizeX, ViewportSizeY);
    if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
    {
        return;
    }

    const FVector2D RandomOffset = MakeDamageNumberRandomOffset();

    ULastFPSDamageNumberWidget* DamageNumberWidget =
        CreateWidget<ULastFPSDamageNumberWidget>(OwningPlayer, Config.DamageNumberWidgetClass);
    if (!DamageNumberWidget)
    {
        return;
    }

    DamageNumberWidget->AddToViewport(20);
    DamageNumberWidget->InitializeDamageNumber(
        DamageAmount,
        TotalDamageDealt,
        DamageTargetActor,
        DamageWorldLocation,
        Config.DamageNumberWorldOffset,
        Config.DamageNumberScreenOffset,
        RandomOffset,
        bCriticalHit);
}

FVector2D ULastFPSCombatFeedbackPresenter::MakeDamageNumberRandomOffset() const
{
    const float RandomAngle = FMath::FRandRange(0.f, 2.f * PI);
    const float RandomDistance =
        Config.DamageNumberRandomRadiusOffset + FMath::FRandRange(0.f, Config.DamageNumberRandomRadius);

    return FVector2D(
        FMath::Cos(RandomAngle) * RandomDistance,
        FMath::Sin(RandomAngle) * RandomDistance);
}

void ULastFPSCombatFeedbackPresenter::Tick(APlayerController* OwningPlayer, float DeltaTime)
{
    TickHitMarkerSpread(DeltaTime);
    TickDamageDirectionIndicators(OwningPlayer, DeltaTime);
}
