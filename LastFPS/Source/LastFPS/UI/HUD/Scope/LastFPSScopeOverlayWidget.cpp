#include "UI/HUD/Scope/LastFPSScopeOverlayWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Localization/LastFPSLocalization.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSScopeOverlay, Log, All);

namespace
{
    constexpr float CentimetersPerMeter = 100.f;
}

void ULastFPSScopeOverlayWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!ScopeVignette)
    {
        UE_LOG(LogLastFPSScopeOverlay, Warning,
            TEXT("스코프 오버레이 '%s'에 ScopeVignette 이미지가 없어 종횡비를 보정할 수 없습니다. "
                 "위젯 BP에 ScopeVignette 이름의 Image를 두세요."),
            *GetNameSafe(this));
        return;
    }

    VignetteMaterial = ScopeVignette->GetDynamicMaterial();
    if (!VignetteMaterial)
    {
        UE_LOG(LogLastFPSScopeOverlay, Warning,
            TEXT("스코프 오버레이 '%s'의 ScopeVignette 브러시에 머티리얼이 없습니다. "
                 "M_ScopeVignette를 지정하세요."),
            *GetNameSafe(this));
    }

    AppliedAspectRatio = 0.f;
    DisplayedRangeMeters = -1;
}

void ULastFPSScopeOverlayWidget::NativeDestruct()
{
    VignetteMaterial = nullptr;
    AppliedAspectRatio = 0.f;

    Super::NativeDestruct();
}

void ULastFPSScopeOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    ApplyAspectRatio(MyGeometry.GetLocalSize());
    UpdateRange();
}

void ULastFPSScopeOverlayWidget::UpdateRange()
{
    if (!RangeText)
    {
        return;
    }

    const APlayerController* PC = GetOwningPlayer();
    if (!PC || !PC->PlayerCameraManager)
    {
        return;
    }

    const FVector Start = PC->PlayerCameraManager->GetCameraLocation();
    const FVector End = Start + PC->PlayerCameraManager->GetCameraRotation().Vector() * (MaxRangeMeters * CentimetersPerMeter);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(ScopeRangefinder), false, PC->GetPawn());
    FHitResult Hit;

    const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    const int32 Meters = bHit ? FMath::RoundToInt(Hit.Distance / CentimetersPerMeter) : -1;
    if (Meters == DisplayedRangeMeters)
    {
        return;
    }

    DisplayedRangeMeters = Meters;
    RangeText->SetText(bHit
        ? FText::Format(
            FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::DistanceMetersFormat),
            FText::AsNumber(Meters))
        : FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::UnknownDistanceMeters));
}

void ULastFPSScopeOverlayWidget::ApplyAspectRatio(const FVector2D& LocalSize)
{
    if (!VignetteMaterial || LocalSize.Y <= 0.f)
    {
        return;
    }

    const float AspectRatio = LocalSize.X / LocalSize.Y;
    if (FMath::IsNearlyEqual(AspectRatio, AppliedAspectRatio, 0.001f))
    {
        return;
    }

    AppliedAspectRatio = AspectRatio;
    VignetteMaterial->SetScalarParameterValue(AspectParameterName, AspectRatio);
}
