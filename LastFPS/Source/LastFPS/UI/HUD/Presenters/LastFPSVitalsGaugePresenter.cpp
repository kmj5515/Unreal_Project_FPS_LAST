#include "UI/HUD/Presenters/LastFPSVitalsGaugePresenter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Components/ProgressBar.h"
#include "Styling/SlateBrush.h"

void FLastFPSSmoothedGaugeDisplay::Initialize(float Current, float InMax)
{
    Target          = Current;
    Displayed       = Current;
    Max             = FMath::Max(InMax, KINDA_SMALL_NUMBER);
    bInterpActive   = false;
}

void FLastFPSSmoothedGaugeDisplay::SetTarget(float NewTarget)
{
    Target = NewTarget;
    bInterpActive = !FMath::IsNearlyEqual(Displayed, Target, KINDA_SMALL_NUMBER);
    if (!bInterpActive)
    {
        Displayed = Target;
    }
}

bool FLastFPSSmoothedGaugeDisplay::Tick(float DeltaTime, float FillDuration)
{
    if (!bInterpActive)
    {
        return false;
    }

    const float FillSpeed = Max / FMath::Max(FillDuration, KINDA_SMALL_NUMBER);
    const float Previous  = Displayed;

    Displayed = FMath::FInterpConstantTo(Displayed, Target, DeltaTime, FillSpeed);

    if (FMath::IsNearlyEqual(Displayed, Target, KINDA_SMALL_NUMBER))
    {
        Displayed     = Target;
        bInterpActive = false;
    }

    return !FMath::IsNearlyEqual(Previous, Displayed, KINDA_SMALL_NUMBER) || !bInterpActive;
}

void ULastFPSVitalsGaugePresenter::Initialize(
    UProgressBar* InHealthBar,
    UProgressBar* InStaminaBar,
    const FLastFPSVitalsGaugeConfig& InConfig)
{
    HealthBar = InHealthBar;
    StaminaBar = InStaminaBar;
    Config = InConfig;

    ApplyGaugeBarBackground(HealthBar);
    ApplyGaugeBarBackground(StaminaBar);
}

void ULastFPSVitalsGaugePresenter::BindToAbilitySystem(UAbilitySystemComponent* ASC, const ULastFPSAttributeSet* AS)
{
    if (!ASC || !AS)
    {
        return;
    }

    if (!bAttributeDelegatesBound)
    {
        ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
            .AddUObject(this, &ULastFPSVitalsGaugePresenter::HandleHealthChanged);
        ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetStaminaAttribute())
            .AddUObject(this, &ULastFPSVitalsGaugePresenter::HandleStaminaChanged);
        bAttributeDelegatesBound = true;
    }

    HealthGauge.Initialize(AS->GetHealth(), AS->GetMaxHealth());
    StaminaGauge.Initialize(AS->GetStamina(), AS->GetMaxStamina());
    ApplyHealthDisplay();
    ApplyStaminaDisplay();
}

void ULastFPSVitalsGaugePresenter::Tick(float DeltaTime)
{
    if (HealthGauge.Tick(DeltaTime, Config.GaugeFillDuration))
    {
        ApplyHealthDisplay();
    }

    if (StaminaGauge.Tick(DeltaTime, Config.GaugeFillDuration))
    {
        ApplyStaminaDisplay();
    }
}

void ULastFPSVitalsGaugePresenter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
    HealthGauge.SetTarget(Data.NewValue);
    if (!HealthGauge.bInterpActive)
    {
        ApplyHealthDisplay();
    }
}

void ULastFPSVitalsGaugePresenter::HandleStaminaChanged(const FOnAttributeChangeData& Data)
{
    StaminaGauge.SetTarget(Data.NewValue);
    if (!StaminaGauge.bInterpActive)
    {
        ApplyStaminaDisplay();
    }
}

bool ULastFPSVitalsGaugePresenter::IsLowResource(float Current, float Max) const
{
    if (Max <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    return (Current / Max) < Config.LowResourceThreshold;
}

FLinearColor ULastFPSVitalsGaugePresenter::ResolveHealthFillColor() const
{
    return IsLowResource(HealthGauge.Displayed, HealthGauge.Max)
        ? Config.HealthLowFillColor
        : Config.HealthFillColor;
}

FLinearColor ULastFPSVitalsGaugePresenter::ResolveStaminaFillColor() const
{
    return IsLowResource(StaminaGauge.Displayed, StaminaGauge.Max)
        ? Config.StaminaLowFillColor
        : Config.StaminaFillColor;
}

void ULastFPSVitalsGaugePresenter::ApplyGaugeBarBackground(UProgressBar* Bar) const
{
    if (!Bar)
    {
        return;
    }

    FProgressBarStyle Style = Bar->GetWidgetStyle();
    Style.BackgroundImage.TintColor = FSlateColor(Config.GaugeBackgroundColor);
    Bar->SetWidgetStyle(Style);
}

void ULastFPSVitalsGaugePresenter::ApplyGaugeBar(
    UProgressBar* Bar,
    float Current,
    float Max,
    const FLinearColor& FillColor) const
{
    if (!Bar)
    {
        return;
    }

    const float Percent = Max > KINDA_SMALL_NUMBER
        ? FMath::Clamp(Current / Max, 0.f, 1.f)
        : 0.f;

    Bar->SetPercent(Percent);
    Bar->SetFillColorAndOpacity(FillColor);
}

void ULastFPSVitalsGaugePresenter::ApplyHealthDisplay()
{
    ApplyGaugeBar(HealthBar, HealthGauge.Displayed, HealthGauge.Max, ResolveHealthFillColor());
}

void ULastFPSVitalsGaugePresenter::ApplyStaminaDisplay()
{
    ApplyGaugeBar(StaminaBar, StaminaGauge.Displayed, StaminaGauge.Max, ResolveStaminaFillColor());
}
