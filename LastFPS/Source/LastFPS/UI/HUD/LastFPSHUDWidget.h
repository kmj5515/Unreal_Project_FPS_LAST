#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Math/Color.h"
#include "UI/HUD/LastFPSEnemyHealthBarWidget.h"
#include "UI/HUD/LastFPSHUDStyle.h"
#include "LastFPSHUDWidget.generated.h"

class UAbilitySystemComponent;
class UecsCrosshairEditorAsset;
class ULastFPSEasyCrosshairPresenter;
class ULastFPSCombatFeedbackPresenter;
class ULastFPSEnemyHealthPresenter;
class ULastFPSGrapplingReticlePresenter;
class ULastFPSVitalsGaugePresenter;
class ULastFPSReloadPresenter;
class ULastFPSSkillCooldownPresenter;
class ULastFPSAmmoPresenter;
class UTextBlock;
class ULastFPSGrapplingTargetingComponent;
class ULastFPSDamageDirectionIndicatorWidget;
class ULastFPSDamageNumberWidget;
class ULastFPSEnemyHealthBarWidget;
class ULastFPSSkillCooldownSlotWidget;
class ULastFPSStatusEffectListWidget;
class UOverlay;
class UUserWidget;
class UWeaponComponent;
class AActor;
class ALastFPSCharacterBase;
class ALastFPSHero;
class ALastFPSPlayerState;

/**
 * 플레이어 HUD의 View(MVP).
 * 바인딩 위젯·디자이너 설정을 소유하고, 표시 로직은 전용 Presenter에 위임한다.
 * - 전투 피드백(히트마커/데미지 넘버/피격 방향): ULastFPSCombatFeedbackPresenter
 * - 적/보스 체력바: ULastFPSEnemyHealthPresenter
 * - 그래플링 조준점: ULastFPSGrapplingReticlePresenter
 * - 체력/스태미나 게이지: ULastFPSVitalsGaugePresenter
 * - 리로드 진행: ULastFPSReloadPresenter
 * - 스킬 쿨다운 슬롯: ULastFPSSkillCooldownPresenter
 * - EasyCrosshair: ULastFPSEasyCrosshairPresenter
 * View 자신은 Presenter 조립·수명 관리·틱 위임과 폰/ASC 바인딩만 담당한다.
 */
UCLASS()
class LASTFPS_API ULastFPSHUDWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()

public:
    ULastFPSHUDWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
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

    UPROPERTY(BlueprintReadOnly, Category="HUD|Scope", meta=(BindWidgetOptional))
    TObjectPtr<UOverlay> ScopeOverlayHost;

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

    /** 리로드 진행을 머티리얼 파라미터로 채우는 이미지다. C++가 진행률(0~1)을 직접 반영한다. */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Reload", meta=(BindWidgetOptional))
    TObjectPtr<UImage> ReloadImage;
    
    UPROPERTY(BlueprintReadOnly,Category="HUD|Reload", meta=(BindWidgetOptional))
    TObjectPtr<UOverlay> ReloadOverlay;
    
    UPROPERTY(BlueprintReadOnly, Transient, Category="HUD|Reload", meta=(BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> AmmoTwinkleAnim;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Ammo", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> TB_CurrentAmmo;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Ammo", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> TB_ReserveAmmo;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Ammo", meta=(BindWidgetOptional))
    TObjectPtr<UOverlay> AmmoInfoLayer;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Ammo", meta=(BindWidgetOptional))
    TObjectPtr<UOverlay> AmmonInfoLayer;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Reload")
    FName ReloadProgressParameterName = TEXT("ReloadProgress");

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

    UFUNCTION()
    void HandleDamageDealt(
        float DamageAmount,
        float TotalDamageDealt,
        FVector DamageWorldLocation,
        AActor* DamageTargetActor,
        bool bCriticalHit);

    void TryBindPawnComponents();
    UOverlay* ResolveCrosshairHost();
    void RefreshEasyCrosshair();
    void RemoveEasyCrosshair();
    void SetEasyCrosshairVisibility(bool bVisible);

    UFUNCTION()
    void HandleWeaponEquippedChanged(bool bEquipped);

    UFUNCTION()
    void HandleAimingChanged(bool bIsAiming);
    void UpdateScopeOverlay(bool bAiming);
    void ClearScopeOverlay();

    /*Reload */
    UFUNCTION()
    void HandleReloadStarted(float ReloadDuration);
    
    UFUNCTION()
    void HandleReloadFinished(bool bCompleted);
    
    UFUNCTION()
    void HandleGrapplingTargetAvailabilityChanged(bool bTargetAvailable);

    /** 바인딩 위젯·설정을 Presenter들에 전달해 초기화한다. NativeConstruct에서 1회 호출한다. */
    void InitializePresenters();

    // 폰/PlayerState가 아직 준비되지 않았을 때 InitializeHUD를 재시도하는 타이머.
    // 매 프레임 갱신은 NativeTick이 담당하므로 별도 갱신 타이머는 두지 않는다.
    FTimerHandle RetryTimerHandle;

    // ── 표시 로직 Presenter (View가 소유하고 조립·틱을 위임한다) ──
    UPROPERTY(Transient)
    TObjectPtr<ULastFPSEasyCrosshairPresenter> CrosshairPresenter;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSCombatFeedbackPresenter> CombatFeedbackPresenter;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSEnemyHealthPresenter> EnemyHealthPresenter;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSGrapplingReticlePresenter> GrapplingReticlePresenter;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSVitalsGaugePresenter> VitalsGaugePresenter;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSReloadPresenter> ReloadPresenter;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSSkillCooldownPresenter> SkillCooldownPresenter;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSAmmoPresenter> AmmoPresenter;

    bool bCrosshairConfigurationWarningLogged = false;

    TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
    TWeakObjectPtr<UWeaponComponent> BoundWeaponComponent;
    TWeakObjectPtr<ULastFPSGrapplingTargetingComponent> BoundGrapplingTargetingComponent;
    TWeakObjectPtr<ALastFPSPlayerState> BoundPlayerState;
    TWeakObjectPtr<ALastFPSHero> BoundHero;

    bool bPawnComponentsBound = false;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> ScopeOverlayWidget;
    TSubclassOf<UUserWidget> ScopeOverlayWidgetClass;
    bool bLastAiming = false;
};
