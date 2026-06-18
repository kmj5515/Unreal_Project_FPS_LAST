#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Math/Color.h"
#include "GameplayEffectTypes.h"
#include "UI/LastFPSHUDStyle.h"
#include "LastFPSHUDWidget.generated.h"

class UAbilitySystemComponent;
class ULastFPSSkillCooldownSlotWidget;
class ULastFPSQuestTrackerWidget;
class UMaterialInstanceDynamic;
class UWeaponComponent;

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

UCLASS()
class LASTFPS_API ULastFPSHUDWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()

public:
    ULastFPSHUDWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

    UFUNCTION(BlueprintCallable, Category="HUD|HitMarker")
    void ShowHitMarker();

    UFUNCTION(BlueprintCallable, Category="HUD|Crosshair")
    void AddCrosshairFireSpread(float SpreadAmount = -1.f);

    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> WBP_SkillCooldownSlot_Q;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> WBP_SkillCooldownSlot_E;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> WBP_SkillCooldownSlot_F;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Quest", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSQuestTrackerWidget> WBP_QuestTracker;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="HUD")
    void OnHealthChanged(float Current, float Max);

    UFUNCTION(BlueprintImplementableEvent, Category="HUD")
    void OnStaminaChanged(float Current, float Max);

    UFUNCTION(BlueprintImplementableEvent, Category="HUD")
    void OnUltimateGaugeChanged(float Current, float Max);

    UFUNCTION(BlueprintImplementableEvent, Category="HUD")
    void OnHeatChanged(float Current, float Max, bool bIsOverheated);

    UFUNCTION(BlueprintImplementableEvent, Category="HUD")
    void OnCrosshairVisibilityChanged(bool bVisible);

    UFUNCTION(BlueprintImplementableEvent, Category="HUD")
    void OnCrosshairSpreadChanged(float Spread);

    UPROPERTY(BlueprintReadOnly, Category="HUD|HitMarker", meta=(BindWidgetOptional))
    TObjectPtr<UImage> HitMarkerImage;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Crosshair", meta=(BindWidgetOptional))
    TObjectPtr<UImage> CrosshairImage;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Crosshair|Material")
    FName CrosshairSpreadParameterName = TEXT("Spread");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Crosshair", meta=(ClampMin="-0.1"))
    float CrosshairBaseSpread = 0.005f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Crosshair", meta=(ClampMin="0.0"))
    float CrosshairMoveSpread = 0.02f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Crosshair", meta=(ClampMin="0.0"))
    float CrosshairJumpSpread = 0.04f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Crosshair", meta=(ClampMin="0.0"))
    float CrosshairFireSpread = 0.025f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Crosshair", meta=(ClampMin="0.0"))
    float CrosshairMaxFireSpread = 0.08f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Crosshair", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CrosshairZoomSpreadMultiplier = 0.3f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Crosshair", meta=(ClampMin="0.0"))
    float CrosshairRecoverSpeed = 12.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Crosshair", meta=(ClampMin="0.0"))
    float CrosshairFireRecoverSpeed = 9.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Crosshair", meta=(ClampMin="0.0"))
    float CrosshairMovementSpeedThreshold = 10.f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|HitMarker|Material")
    FName HitMarkerSpreadParameterName = TEXT("HitSpread");

    UPROPERTY(EditDefaultsOnly, Category="HUD|HitMarker|Material", meta=(ClampMin="0.0"))
    float HitMarkerMaxSpread = 5.f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|HitMarker|Material", meta=(ClampMin="0.01", ClampMax="1.0"))
    float HitMarkerSpreadExpandDuration = 0.12f;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Gauges", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Health;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Gauges", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Stamina;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Gauges", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Ultimate;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Gauges", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Heat;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges", meta=(ClampMin="0.05", ClampMax="3.0"))
    float GaugeFillDuration = 0.4f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    float LowResourceThreshold = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    FLinearColor GaugeBackgroundColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    FLinearColor HealthFillColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    FLinearColor HealthLowFillColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    FLinearColor StaminaFillColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    FLinearColor StaminaLowFillColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    FLinearColor UltimateFillColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    FLinearColor UltimateReadyFillColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    FLinearColor HeatFillColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges|Colors")
    FLinearColor HeatOverheatedFillColor;

private:
    bool InitializeHUD();

    UFUNCTION()
    void RetryInitialize();

    void HandleHealthChanged(const FOnAttributeChangeData& Data);
    void HandleStaminaChanged(const FOnAttributeChangeData& Data);
    void HandleUltimateGaugeChanged(const FOnAttributeChangeData& Data);

    bool TryInitSkillSlots();
    void TickSkillSlots();
    void HUDRefreshTick(float DeltaTime);
    void HUDRefreshTickFromTimer();

    void TryBindPawnComponents();
    void TickSmoothedGauges(float DeltaTime);
    void InitializeHitMarkerMaterial();
    void TickHitMarkerSpread(float DeltaTime);
    void SetHitMarkerSpread(float Spread);
    void InitializeCrosshairMaterial();
    void TickCrosshairSpread(float DeltaTime);
    void SetCrosshairSpread(float Spread);
    void BroadcastHealthDisplay();
    void BroadcastStaminaDisplay();
    void BroadcastUltimateGaugeDisplay();
    void BroadcastHeatDisplay();
    void ApplyGaugeBarBackground(UProgressBar* Bar) const;
    void ApplyGaugeBar(UProgressBar* Bar, float Current, float Max, const FLinearColor& FillColor) const;
    FLinearColor ResolveHealthFillColor() const;
    FLinearColor ResolveStaminaFillColor() const;
    FLinearColor ResolveUltimateFillColor() const;
    FLinearColor ResolveHeatFillColor() const;
    bool IsLowResource(float Current, float Max) const;

    UFUNCTION()
    void HandleHeatChanged(float Current, float Max, bool bIsOverheated);

    UFUNCTION()
    void HandleWeaponEquippedChanged(bool bEquipped);

    FTimerHandle RetryTimerHandle;
    FTimerHandle HUDRefreshTimerHandle;
    void HideHitMarker();

    TWeakObjectPtr<UMaterialInstanceDynamic> HitMarkerMaterial;
    TWeakObjectPtr<UMaterialInstanceDynamic> CrosshairMaterial;
    float HitMarkerSpreadElapsed = 0.f;
    bool bHitMarkerSpreadAnimating = false;
    float CurrentCrosshairSpread = 0.f;
    float FireCrosshairSpread = 0.f;

    FLastFPSSmoothedGaugeDisplay HealthGauge;
    FLastFPSSmoothedGaugeDisplay StaminaGauge;
    FLastFPSSmoothedGaugeDisplay UltimateGauge;
    FLastFPSSmoothedGaugeDisplay HeatGauge;
    float CachedMaxHeat = 0.f;
    bool CachedHeatOverheated = false;

    TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

    bool bAttributeDelegatesBound = false;
    bool bPawnComponentsBound = false;
    bool bSkillSlotsInitialized = false;
};
