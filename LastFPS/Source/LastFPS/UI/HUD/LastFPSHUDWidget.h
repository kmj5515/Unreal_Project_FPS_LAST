#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Math/Color.h"
#include "GameplayEffectTypes.h"
#include "UI/HUD/LastFPSEnemyHealthBarWidget.h"
#include "UI/HUD/LastFPSHUDStyle.h"
#include "LastFPSHUDWidget.generated.h"

class UAbilitySystemComponent;
class UecsCrosshairEditorAsset;
class ULastFPSEasyCrosshairPresenter;
class ULastFPSGrapplingTargetingComponent;
class ULastFPSDamageDirectionIndicatorWidget;
class ULastFPSDamageNumberWidget;
class ULastFPSEnemyHealthBarWidget;
class ULastFPSSkillCooldownSlotWidget;
class ULastFPSStatusEffectListWidget;
class UMaterialInstanceDynamic;
class UOverlay;
class UWeaponComponent;
class AActor;
class ALastFPSCharacterBase;
class ALastFPSPlayerState;

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

    /** 월드 기준 공격 방향을 카메라 기준 화면 방향으로 표시한다. */
    void ShowDamageDirection(const FVector& DamageSourceDirection);

    UFUNCTION(BlueprintCallable, Category="HUD|Crosshair")
    void PlayCrosshairFireAnimation();

    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> WBP_SkillCooldownSlot_Q;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> WBP_SkillCooldownSlot_E;
    
    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> WBP_SkillCooldownSlot_Z;
    
    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> WBP_SkillCooldownSlot_F;

    /** 버프·디버프 아이콘 목록이다. 위젯 블루프린트의 동일한 이름과 선택적으로 바인딩한다. */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Status Effect", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSStatusEffectListWidget> WBP_StatusEffectList;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="HUD")
    void OnHealthChanged(float Current, float Max);

    UFUNCTION(BlueprintImplementableEvent, Category="HUD")
    void OnStaminaChanged(float Current, float Max);

    UPROPERTY(BlueprintReadOnly, Category="HUD|HitMarker", meta=(BindWidgetOptional))
    TObjectPtr<UImage> HitMarkerImage;

    /** 동적으로 생성한 공격 방향 위젯을 쌓는 전체 화면 레이어다. */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Damage Direction", meta=(BindWidgetOptional))
    TObjectPtr<UOverlay> DamageDirectionIndicatorLayer;

    /** 무기나 캐릭터와 무관한 표시 모양과 수명 설정은 위젯 클래스 기본값에서 관리한다. */
    UPROPERTY(EditDefaultsOnly, Category="HUD|Damage Direction")
    TSubclassOf<ULastFPSDamageDirectionIndicatorWidget> DamageDirectionIndicatorWidgetClass;

    /** 짧은 시간에 누적되는 위젯 수를 제한해 UI 객체가 무한히 증가하지 않게 한다. */
    UPROPERTY(EditDefaultsOnly, Category="HUD|Damage Direction", meta=(ClampMin="1", ClampMax="16"))
    int32 MaxDamageDirectionIndicators = 6;

    /** 지정하면 EasyCrosshair가 이 Overlay 안에 배치된다. 비어 있으면 런타임에 HUD 루트에 생성한다. */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Crosshair", meta=(BindWidgetOptional))
    TObjectPtr<UOverlay> CrosshairHost;

    /** EasyCrosshair 아래에서 항상 표시되는 그래플링 가능 상태 점이다. 없으면 런타임에 생성한다. */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Grappling Reticle", meta=(BindWidgetOptional))
    TObjectPtr<UImage> GrapplingDotImage;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Grappling Reticle")
    float GrapplingDotSize = 6.f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Grappling Reticle", meta=(ClampMin="0.01"))
    float GrapplingDotIdleScale = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Grappling Reticle", meta=(ClampMin="0.01"))
    float GrapplingDotAvailableScale = 1.7f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Grappling Reticle", meta=(ClampMin="0.0"))
    float GrapplingDotScaleInterpSpeed = 12.f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Grappling Reticle")
    FLinearColor GrapplingDotIdleColor = FLinearColor(1.f, 1.f, 1.f, 0.55f);

    UPROPERTY(EditDefaultsOnly, Category="HUD|Grappling Reticle")
    FLinearColor GrapplingDotAvailableColor = FLinearColor(0.15f, 0.9f, 1.f, 1.f);

    /** 무기 Definition에 전용 에셋이 없을 때 사용하는 기본 EasyCrosshair 에셋이다. */
    UPROPERTY(EditDefaultsOnly, Category="HUD|Crosshair")
    TSoftObjectPtr<UecsCrosshairEditorAsset> DefaultCrosshairAsset;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Crosshair")
    FName DefaultCrosshairFireAnimationName = TEXT("Shoot");

    /** 0이면 EasyCrosshair 에셋에 저장된 애니메이션 시간을 사용한다. */
    UPROPERTY(EditDefaultsOnly, Category="HUD|Crosshair", meta=(ClampMin="0.0", Units="s"))
    float DefaultCrosshairFireAnimationDuration = 0.f;

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

    /** 리로드 진행을 표시하는 프로그레스 바다. C++가 진행률을 직접 채운다. */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Reload", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Reload;

    /** 리로드 남은 시간을 숫자로 표시하는 텍스트다. 선택적으로 바인딩하며 C++가 갱신한다. */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Reload", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Reload;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Reload|Colors")
    FLinearColor ReloadFillColor = FLinearColor(0.15f, 0.6f, 1.f, 1.f);

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

    UPROPERTY(EditDefaultsOnly, Category="HUD|Damage")
    TSubclassOf<ULastFPSDamageNumberWidget> DamageNumberWidgetClass;

    /** 피해를 준 적만 제한적으로 표시하여 다수의 적이 있어도 UI 비용이 일정하게 유지되도록 한다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Enemy Health")
    FLastFPSEnemyHealthBarSettings EnemyHealthBarSettings;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Damage")
    FVector DamageNumberWorldOffset = FVector(0.f, 0.f, 55.f);

    UPROPERTY(EditDefaultsOnly, Category="HUD|Damage")
    FVector2D DamageNumberScreenOffset = FVector2D(90.f, -20.f);

    UPROPERTY(EditDefaultsOnly, Category="HUD|Damage", meta=(ClampMin="0.0"))
    float DamageNumberRandomRadius = 24.f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Damage", meta=(ClampMin="0.0"))
    float DamageNumberRandomRadiusOffset = 0.f;

    /** 화면에 고정되어 보스가 죽을 때까지 유지되는 전용 체력바다. */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Boss Health", meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSEnemyHealthBarWidget> WBP_BossHealthBar;
    
private:
    bool InitializeHUD();

    UFUNCTION()
    void RetryInitialize();

    void HandleHealthChanged(const FOnAttributeChangeData& Data);
    void HandleStaminaChanged(const FOnAttributeChangeData& Data);

    UFUNCTION()
    void HandleDamageDealt(
        float DamageAmount,
        float TotalDamageDealt,
        FVector DamageWorldLocation,
        AActor* DamageTargetActor,
        bool bCriticalHit);

    bool TryInitSkillSlots();
    void TickSkillSlots();
    void HUDRefreshTick(float DeltaTime);
    void HUDRefreshTickFromTimer();

    void TryBindPawnComponents();
    void TickSmoothedGauges(float DeltaTime);
    void InitializeHitMarkerMaterial();
    void TickHitMarkerSpread(float DeltaTime);
    void SetHitMarkerSpread(float Spread);
    void TickDamageDirectionIndicators(float DeltaTime);
    void ClearDamageDirectionIndicators();
    UOverlay* ResolveCrosshairHost();
    void RefreshEasyCrosshair();
    void RemoveEasyCrosshair();
    void SetEasyCrosshairVisibility(bool bVisible);
    void EnsureGrapplingDot();
    void TickGrapplingDot(float DeltaTime);
    void ApplyGrapplingDotAvailability(bool bTargetAvailable);
    void BroadcastHealthDisplay();
    void BroadcastStaminaDisplay();
    void SpawnDamageNumber(
        float DamageAmount,
        float TotalDamageDealt,
        const FVector& DamageWorldLocation,
        AActor* DamageTargetActor,
        bool bCriticalHit);
    void ShowEnemyHealthBar(AActor* DamageTargetActor, float DamageAmount);
    void ReleaseEnemyHealthBarFor(const ALastFPSCharacterBase* Enemy);
    void TickEnemyHealthBars(float DeltaTime);
    void ClearEnemyHealthBars();
    void TickBossHealthBar(float DeltaTime);
    void ClearBossHealthBar();
    FVector2D MakeDamageNumberRandomOffset() const;
    void ApplyGaugeBarBackground(UProgressBar* Bar) const;
    void ApplyGaugeBar(UProgressBar* Bar, float Current, float Max, const FLinearColor& FillColor) const;
    FLinearColor ResolveHealthFillColor() const;
    FLinearColor ResolveStaminaFillColor() const;
    bool IsLowResource(float Current, float Max) const;

    UFUNCTION()
    void HandleWeaponEquippedChanged(bool bEquipped);

    UFUNCTION()
    void HandleReloadStarted(float ReloadDuration);

    UFUNCTION()
    void HandleReloadFinished(bool bCompleted);

    void TickReloadIndicator(float DeltaTime);
    void SetReloadIndicatorVisible(bool bVisible);
    // 리로드 진행률(0~1)과 남은 시간(초)을 프로그레스 바·텍스트에 직접 반영한다.
    void UpdateReloadDisplay(float Progress, float RemainingSeconds);

    UFUNCTION()
    void HandleGrapplingTargetAvailabilityChanged(bool bTargetAvailable);

    FTimerHandle RetryTimerHandle;
    FTimerHandle HUDRefreshTimerHandle;
    void HideHitMarker();

    TWeakObjectPtr<UMaterialInstanceDynamic> HitMarkerMaterial;
    UPROPERTY(Transient)
    TObjectPtr<ULastFPSEasyCrosshairPresenter> CrosshairPresenter;
    
    float HitMarkerSpreadElapsed = 0.f;
    bool bHitMarkerSpreadAnimating = false;
    
    UPROPERTY(Transient)
    TArray<TObjectPtr<ULastFPSDamageDirectionIndicatorWidget>> ActiveDamageDirectionIndicators;

    UPROPERTY(Transient)
    TArray<TObjectPtr<ULastFPSEnemyHealthBarWidget>> EnemyHealthBarPool;
    
    bool bDamageDirectionConfigurationWarningLogged = false;
    bool bCrosshairConfigurationWarningLogged = false;

    FLastFPSSmoothedGaugeDisplay HealthGauge;
    FLastFPSSmoothedGaugeDisplay StaminaGauge;

    TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
    TWeakObjectPtr<UWeaponComponent> BoundWeaponComponent;
    TWeakObjectPtr<ULastFPSGrapplingTargetingComponent> BoundGrapplingTargetingComponent;
    TWeakObjectPtr<ALastFPSPlayerState> BoundPlayerState;

    float GrapplingDotCurrentScale = 1.f;
    float GrapplingDotTargetScale = 1.f;
    bool bGrapplingDotInitialized = false;

    bool bAttributeDelegatesBound = false;
    bool bPawnComponentsBound = false;
    bool bSkillSlotsInitialized = false;

    // 리로드 표시 상태. 진행 시간은 시작 알림의 소요 시간을 기준으로 HUD 틱에서 자체 누적한다.
    bool bReloadInProgress = false;
    float ReloadElapsedSeconds = 0.f;
    float ReloadTotalSeconds = 0.f;
};
