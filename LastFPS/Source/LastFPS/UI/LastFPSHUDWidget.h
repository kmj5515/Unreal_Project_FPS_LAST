#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GameplayEffectTypes.h"
#include "LastFPSHUDWidget.generated.h"

class UAbilitySystemComponent;
class UWeaponComponent;

UCLASS()
class LASTFPS_API ULastFPSHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
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

private:
    // 성공 시 true 반환. PlayerState 미준비면 false → 타이머 재시도
    bool InitializeHUD();

    UFUNCTION()
    void RetryInitialize();

    void HandleHealthChanged(const FOnAttributeChangeData& Data);
    void HandleStaminaChanged(const FOnAttributeChangeData& Data);
    void HandleUltimateGaugeChanged(const FOnAttributeChangeData& Data);
    void HandleKillFeed(const FString& KillerName, const FString& VictimName);
    void TryBindKillFeed();
    void AddKillFeedLine(const FString& KillerName, const FString& VictimName);

    UFUNCTION()
    void HandleHeatChanged(float Current, float Max, bool bIsOverheated);

    UFUNCTION()
    void HandleWeaponEquippedChanged(bool bEquipped);

    FTimerHandle RetryTimerHandle;
    FTimerHandle HitMarkerTimerHandle;

    void HideHitMarker();

    float CachedMaxHealth        = 0.f;
    float CachedMaxStamina       = 0.f;
    float CachedMaxUltimateGauge = 0.f;

    // 1초 단위로 1번만 BP에 통지하기 위한 캐시 (-1: 아직 미통지)
    int32 CachedMatchTimeIntSec  = -1;

    TWeakObjectPtr<class ALastFPSMatchGameState> BoundMatchGameState;
    FDelegateHandle KillFeedHandle;
    TArray<TObjectPtr<UTextBlock>> KillFeedLines;
};
