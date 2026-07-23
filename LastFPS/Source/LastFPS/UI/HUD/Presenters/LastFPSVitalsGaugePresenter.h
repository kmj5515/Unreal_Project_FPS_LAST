#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayEffectTypes.h"
#include "LastFPSVitalsGaugePresenter.generated.h"

class UAbilitySystemComponent;
class ULastFPSAttributeSet;
class UProgressBar;

/**
 * 게이지 표시값을 목표값으로 매끄럽게 채우기 위한 보간 상태.
 * 어트리뷰트 변경은 즉시 반영되지 않고 FillDuration에 맞춰 점진적으로 따라간다.
 */
struct FLastFPSSmoothedGaugeDisplay
{
    float Target = 0.f;
    float Displayed = 0.f;
    float Max = 1.f;
    bool bInterpActive = false;

    void Initialize(float Current, float InMax);
    void SetTarget(float NewTarget);
    bool Tick(float DeltaTime, float FillDuration);
};

/** 체력·스태미나 게이지 색·보간 설정. 디자이너 노출 값은 View에서 관리하고 초기화 시 전달한다. */
struct FLastFPSVitalsGaugeConfig
{
    float GaugeFillDuration = 0.4f;
    float LowResourceThreshold = 0.25f;
    FLinearColor GaugeBackgroundColor = FLinearColor::Black;
    FLinearColor HealthFillColor = FLinearColor::Green;
    FLinearColor HealthLowFillColor = FLinearColor::Red;
    FLinearColor StaminaFillColor = FLinearColor::Yellow;
    FLinearColor StaminaLowFillColor = FLinearColor(1.f, 0.5f, 0.f, 1.f);
};

/**
 * 플레이어 체력·스태미나 게이지 표시를 HUD View에서 분리한다.
 * 어트리뷰트 변경 델리게이트를 직접 구독하고, 표시값을 매끄럽게 보간해 프로그레스 바에 반영한다.
 */
UCLASS()
class LASTFPS_API ULastFPSVitalsGaugePresenter final : public UObject
{
    GENERATED_BODY()

public:
    /** 프로그레스 바 위젯(null 허용)과 설정을 받아 배경을 구성한다. */
    void Initialize(UProgressBar* InHealthBar, UProgressBar* InStaminaBar, const FLastFPSVitalsGaugeConfig& InConfig);

    /** 어트리뷰트 변경 델리게이트를 구독하고 현재 값으로 게이지를 초기화한다. 중복 구독은 무시한다. */
    void BindToAbilitySystem(UAbilitySystemComponent* ASC, const ULastFPSAttributeSet* AS);

    /** 표시값을 목표값으로 매끄럽게 보간한다. */
    void Tick(float DeltaTime);

private:
    void HandleHealthChanged(const FOnAttributeChangeData& Data);
    void HandleStaminaChanged(const FOnAttributeChangeData& Data);
    void ApplyHealthDisplay();
    void ApplyStaminaDisplay();
    void ApplyGaugeBarBackground(UProgressBar* Bar) const;
    void ApplyGaugeBar(UProgressBar* Bar, float Current, float Max, const FLinearColor& FillColor) const;
    FLinearColor ResolveHealthFillColor() const;
    FLinearColor ResolveStaminaFillColor() const;
    bool IsLowResource(float Current, float Max) const;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> StaminaBar;

    FLastFPSVitalsGaugeConfig Config;
    FLastFPSSmoothedGaugeDisplay HealthGauge;
    FLastFPSSmoothedGaugeDisplay StaminaGauge;

    bool bAttributeDelegatesBound = false;
};
