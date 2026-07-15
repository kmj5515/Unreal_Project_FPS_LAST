#include "UI/HUD/LastFPSDamageDirectionIndicatorWidget.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSDamageDirectionIndicator, Log, All);

void ULastFPSDamageDirectionIndicatorWidget::NativeConstruct()
{
    Super::NativeConstruct();

    InitializeDamageDirectionMaterial();
}

void ULastFPSDamageDirectionIndicatorWidget::NativeDestruct()
{
    DamageDirectionMaterial.Reset();
    DamageSourceDirection = FVector::ZeroVector;
    ElapsedTime = 0.f;

    Super::NativeDestruct();
}

bool ULastFPSDamageDirectionIndicatorWidget::InitializeDamageDirection(const FVector& InDamageSourceDirection)
{
    if (!DamageDirectionIndicatorRotationRoot || !DamageDirectionIndicatorImage)
    {
        UE_LOG(
            LogLastFPSDamageDirectionIndicator,
            Warning,
            TEXT("공격 방향 위젯 '%s'을 초기화하지 못했습니다: 필수 위젯 바인딩을 확인하세요."),
            *GetNameSafe(this));
        return false;
    }

    const FVector HorizontalDirection(InDamageSourceDirection.X, InDamageSourceDirection.Y, 0.f);
    DamageSourceDirection = HorizontalDirection.GetSafeNormal();
    if (DamageSourceDirection.IsNearlyZero())
    {
        UE_LOG(
            LogLastFPSDamageDirectionIndicator,
            Verbose,
            TEXT("공격 방향 위젯 '%s'을 표시하지 않았습니다: 수평 공격 방향이 유효하지 않습니다."),
            *GetNameSafe(this));
        return false;
    }

    ElapsedTime = 0.f;
    InitializeDamageDirectionMaterial();
    SetDamageDirectionProgress(1.f);
    SetRenderOpacity(1.f);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    return true;
}

bool ULastFPSDamageDirectionIndicatorWidget::AdvanceIndicator(
    const float DeltaTime,
    const FRotator& ViewRotation)
{
    if (DamageSourceDirection.IsNearlyZero())
    {
        return false;
    }

    ElapsedTime += FMath::Max(DeltaTime, 0.f);
    const float ShrinkDuration = FMath::Max(DamageDirectionShrinkDuration, KINDA_SMALL_NUMBER);
    const float HoldDuration = FMath::Max(DamageDirectionHoldDuration, 0.f);
    if (ElapsedTime >= ShrinkDuration + HoldDuration)
    {
        return false;
    }

    const float MinimumProgress = FMath::Clamp(DamageDirectionMinimumProgress, 0.f, 1.f);
    const float ShrinkAlpha = FMath::Clamp(ElapsedTime / ShrinkDuration, 0.f, 1.f);
    SetDamageDirectionProgress(FMath::Lerp(1.f, MinimumProgress, ShrinkAlpha));
    UpdateDamageDirectionAngle(ViewRotation);
    return true;
}

void ULastFPSDamageDirectionIndicatorWidget::InitializeDamageDirectionMaterial()
{
    if (!DamageDirectionIndicatorImage || DamageDirectionMaterial.IsValid())
    {
        return;
    }

    DamageDirectionMaterial = DamageDirectionIndicatorImage->GetDynamicMaterial();
    if (!DamageDirectionMaterial.IsValid() && !bMaterialWarningLogged)
    {
        UE_LOG(
            LogLastFPSDamageDirectionIndicator,
            Warning,
            TEXT("공격 방향 위젯 '%s'의 이미지에서 동적 머티리얼을 만들지 못했습니다: Brush에 UI 머티리얼을 지정하세요."),
            *GetNameSafe(this));
        bMaterialWarningLogged = true;
    }
}

void ULastFPSDamageDirectionIndicatorWidget::SetDamageDirectionProgress(const float Progress)
{
    InitializeDamageDirectionMaterial();

    if (UMaterialInstanceDynamic* Material = DamageDirectionMaterial.Get())
    {
        Material->SetScalarParameterValue(
            DamageDirectionProgressParameterName,
            FMath::Clamp(Progress, 0.f, 1.f));
    }
}

void ULastFPSDamageDirectionIndicatorWidget::UpdateDamageDirectionAngle(const FRotator& ViewRotation)
{
    if (!DamageDirectionIndicatorRotationRoot || DamageSourceDirection.IsNearlyZero())
    {
        return;
    }

    const FRotator ViewYawRotation(0.f, ViewRotation.Yaw, 0.f);
    const FVector ViewForward = ViewYawRotation.Vector();
    const FVector ViewRight = FRotationMatrix(ViewYawRotation).GetUnitAxis(EAxis::Y);
    const float ForwardAmount = FVector::DotProduct(ViewForward, DamageSourceDirection);
    const float RightAmount = FVector::DotProduct(ViewRight, DamageSourceDirection);
    const float ScreenAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(RightAmount, ForwardAmount));

    DamageDirectionIndicatorRotationRoot->SetRenderTransformAngle(ScreenAngleDegrees);
}
