#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Math/Color.h"
#include "GameplayEffectTypes.h"
#include "UI/LastFPSHUDStyle.h"
#include "LastFPSHUDWidget.generated.h"

class UAbilitySystemComponent;
class UWeaponComponent;

/** HUD Progress Bar 표시용 보간 (실제 gameplay 값과 분리) */
struct FLastFPSSmoothedGaugeDisplay
{
    float Target = 0.f;
    float Displayed = 0.f;
    float Max = 1.f;
    bool bInterpActive = false;

    void Initialize(float Current, float InMax);
    void SetTarget(float NewTarget);
    /** @return true if Displayed 값이 갱신되어 BP 통지가 필요함 */
    bool Tick(float DeltaTime, float FillDuration);
};

UCLASS()
class LASTFPS_API ULastFPSHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    ULastFPSHUDWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category="HUD|HitMarker")
    void ShowHitMarker();

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

    UFUNCTION(BlueprintImplementableEvent, Category="HUD|KillFeed")
    void OnKillFeedEntry(const FString& KillerName, const FString& VictimName);

    UPROPERTY(BlueprintReadOnly, Category="HUD|HitMarker", meta=(BindWidgetOptional))
    TObjectPtr<UImage> HitMarkerImage;

    UPROPERTY(EditDefaultsOnly, Category="HUD|HitMarker", meta=(ClampMin="0.01", ClampMax="2.0"))
    float HitMarkerDisplayDuration = 0.15f;

    /** WBP_HUD에서 동일 이름 TextBlock을 만들면 자동 바인딩되어 매치 남은 시간(MM:SS)이 표시됨 */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Match", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> MatchTimerText;

    /** WBP_HUD에 Vertical Box 이름 KillFeedContainer — 없으면 BP 이벤트만 호출 */
    UPROPERTY(BlueprintReadOnly, Category="HUD|KillFeed", meta=(BindWidgetOptional))
    TObjectPtr<UPanelWidget> KillFeedContainer;

    UPROPERTY(EditDefaultsOnly, Category="HUD|KillFeed", meta=(ClampMin="1", ClampMax="10"))
    int32 MaxKillFeedEntries = 5;

    UPROPERTY(EditDefaultsOnly, Category="HUD|KillFeed|Colors")
    FLinearColor KillFeedKillerColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|KillFeed|Colors")
    FLinearColor KillFeedVictimColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|KillFeed|Colors")
    FLinearColor KillFeedSeparatorColor;

    UPROPERTY(EditDefaultsOnly, Category="HUD|KillFeed|Colors")
    FLinearColor KillFeedLocalPlayerColor;

    /** 0→최대까지 채우거나 비우는 데 걸리는 시간(초). Health/Stamina/Ultimate/Heat 표시 공통 */
    UPROPERTY(EditDefaultsOnly, Category="HUD|Gauges", meta=(ClampMin="0.05", ClampMax="3.0"))
    float GaugeFillDuration = 0.4f;

    /** WBP_HUD Progress Bar 이름과 일치 시 Percent·Fill Color 자동 적용 */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Gauges", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Health;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Gauges", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Stamina;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Gauges", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Ultimate;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Gauges", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Heat;

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
    // 성공 시 true 반환. PlayerState 미준비면 false → 타이머 재시도
    bool InitializeHUD();

    UFUNCTION()
    void RetryInitialize();

    void HandleHealthChanged(const FOnAttributeChangeData& Data);
    void HandleStaminaChanged(const FOnAttributeChangeData& Data);
    void HandleUltimateGaugeChanged(const FOnAttributeChangeData& Data);
    void TickSmoothedGauges(float DeltaTime);
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
    void HandleKillFeed(const FString& KillerName, const FString& VictimName);
    void TryBindKillFeed();
    void AddKillFeedLine(const FString& KillerName, const FString& VictimName);
    UTextBlock* CreateKillFeedText(const FString& Text, const FLinearColor& Color);
    FString GetLocalKillFeedDisplayName() const;

    UFUNCTION()
    void HandleHeatChanged(float Current, float Max, bool bIsOverheated);

    UFUNCTION()
    void HandleWeaponEquippedChanged(bool bEquipped);

    FTimerHandle RetryTimerHandle;
    FTimerHandle HitMarkerTimerHandle;

    void HideHitMarker();

    FLastFPSSmoothedGaugeDisplay HealthGauge;
    FLastFPSSmoothedGaugeDisplay StaminaGauge;
    FLastFPSSmoothedGaugeDisplay UltimateGauge;
    FLastFPSSmoothedGaugeDisplay HeatGauge;
    float CachedMaxHeat = 0.f;
    bool CachedHeatOverheated = false;

    // 1초 단위로 1번만 BP에 통지하기 위한 캐시 (-1: 아직 미통지)
    int32 CachedMatchTimeIntSec  = -1;

    TWeakObjectPtr<class ALastFPSMatchGameState> BoundMatchGameState;
    FDelegateHandle KillFeedHandle;
    TArray<TObjectPtr<UWidget>> KillFeedLines;
};
