
#include "UI/HUD/LastFPSHUDWidget.h"
#include "UI/HUD/LastFPSEnemyHealthBarWidget.h"
#include "UI/HUD/LastFPSEasyCrosshairPresenter.h"
#include "UI/HUD/Presenters/LastFPSCombatFeedbackPresenter.h"
#include "UI/HUD/Presenters/LastFPSEnemyHealthPresenter.h"
#include "UI/HUD/Presenters/LastFPSGrapplingReticlePresenter.h"
#include "UI/HUD/Presenters/LastFPSVitalsGaugePresenter.h"
#include "UI/HUD/Presenters/LastFPSReloadPresenter.h"
#include "UI/HUD/Presenters/LastFPSSkillCooldownPresenter.h"
#include "UI/HUD/Presenters/LastFPSAmmoPresenter.h"
#include "UI/HUD/Presenters/LastFPSObjectiveHudPresenter.h"
#include "UI/HUD/LastFPSCaptureObjectiveWidget.h"
#include "UI/HUD/LastFPSDefendObjectiveWidget.h"
#include "UI/LastFPSDamageNumberWidget.h"
#include "UI/HUD/LastFPSStatusEffectListWidget.h"
#include "UI/HUD/LastFPSHUDStyle.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "GameFramework/PlayerController.h"
#include "PrimaryGameLayout.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "Game/LastFPSPlayerState.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/LastFPSGrapplingTargetingComponent.h"
#include "Character/Components/WeaponComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Animation/WidgetAnimation.h"


DEFINE_LOG_CATEGORY_STATIC(LogLastFPSHUDWidget, Log, All);

ULastFPSHUDWidget::ULastFPSHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    GaugeBackgroundColor     = LastFPSHUDStyle::GaugeBackground();
    HealthFillColor          = LastFPSHUDStyle::HealthFill();
    HealthLowFillColor       = LastFPSHUDStyle::HealthLowFill();
    StaminaFillColor         = LastFPSHUDStyle::StaminaFill();
    StaminaLowFillColor      = LastFPSHUDStyle::StaminaLowFill();
    DamageNumberWidgetClass  = ULastFPSDamageNumberWidget::StaticClass();
    EnemyHealthBarSettings.WidgetClass = ULastFPSEnemyHealthBarWidget::StaticClass();
}

void ULastFPSHUDWidget::InitializePresenters()
{
    CombatFeedbackPresenter = NewObject<ULastFPSCombatFeedbackPresenter>(this);
    FLastFPSCombatFeedbackConfig CombatConfig;
    CombatConfig.HitMarkerSpreadParameterName    = HitMarkerSpreadParameterName;
    CombatConfig.HitMarkerMaxSpread              = HitMarkerMaxSpread;
    CombatConfig.HitMarkerSpreadExpandDuration   = HitMarkerSpreadExpandDuration;
    CombatConfig.DamageNumberWidgetClass         = DamageNumberWidgetClass;
    CombatConfig.DamageNumberWorldOffset         = DamageNumberWorldOffset;
    CombatConfig.DamageNumberScreenOffset        = DamageNumberScreenOffset;
    CombatConfig.DamageNumberRandomRadius        = DamageNumberRandomRadius;
    CombatConfig.DamageNumberRandomRadiusOffset  = DamageNumberRandomRadiusOffset;
    CombatConfig.DamageDirectionIndicatorWidgetClass = DamageDirectionIndicatorWidgetClass;
    CombatConfig.MaxDamageDirectionIndicators    = MaxDamageDirectionIndicators;
    CombatFeedbackPresenter->Initialize(HitMarkerImage, DamageDirectionIndicatorLayer, CombatConfig);

    EnemyHealthPresenter = NewObject<ULastFPSEnemyHealthPresenter>(this);
    EnemyHealthPresenter->Initialize(WBP_BossHealthBar, EnemyHealthBarSettings);

    // 점령·방어·보스는 표시 슬롯 하나를 나눠 쓰므로 전환을 한 곳에서 관리한다.
    ObjectiveHudPresenter = NewObject<ULastFPSObjectiveHudPresenter>(this);
    ObjectiveHudPresenter->Initialize(WBP_DefendObjective, WBP_CaptureObjective);
    ObjectiveHudPresenter->BindToWorld(GetWorld());

    GrapplingReticlePresenter = NewObject<ULastFPSGrapplingReticlePresenter>(this);
    FLastFPSGrapplingReticleConfig GrapplingConfig;
    GrapplingConfig.DotSize          = GrapplingDotSize;
    GrapplingConfig.IdleScale        = GrapplingDotIdleScale;
    GrapplingConfig.AvailableScale   = GrapplingDotAvailableScale;
    GrapplingConfig.ScaleInterpSpeed = GrapplingDotScaleInterpSpeed;
    GrapplingConfig.IdleColor        = GrapplingDotIdleColor;
    GrapplingConfig.AvailableColor   = GrapplingDotAvailableColor;
    GrapplingReticlePresenter->Initialize(GrapplingDotImage, WidgetTree, GrapplingConfig);
    GrapplingReticlePresenter->EnsureDot(ResolveCrosshairHost());

    VitalsGaugePresenter = NewObject<ULastFPSVitalsGaugePresenter>(this);
    FLastFPSVitalsGaugeConfig GaugeConfig;
    GaugeConfig.GaugeFillDuration    = GaugeFillDuration;
    GaugeConfig.LowResourceThreshold = LowResourceThreshold;
    GaugeConfig.GaugeBackgroundColor = GaugeBackgroundColor;
    GaugeConfig.HealthFillColor      = HealthFillColor;
    GaugeConfig.HealthLowFillColor   = HealthLowFillColor;
    GaugeConfig.StaminaFillColor     = StaminaFillColor;
    GaugeConfig.StaminaLowFillColor  = StaminaLowFillColor;
    VitalsGaugePresenter->Initialize(PB_Health, PB_Stamina, GaugeConfig);

    ReloadPresenter = NewObject<ULastFPSReloadPresenter>(this);
    ReloadPresenter->Initialize(ReloadImage,ReloadOverlay,ReloadProgressParameterName);

    SkillCooldownPresenter = NewObject<ULastFPSSkillCooldownPresenter>(this);
    SkillCooldownPresenter->Initialize(
        WBP_SkillCooldownSlot_Q, WBP_SkillCooldownSlot_E, WBP_SkillCooldownSlot_Z, WBP_SkillCooldownSlot_F);

    AmmoPresenter = NewObject<ULastFPSAmmoPresenter>(this);
    UOverlay* InfoLayer = AmmoInfoLayer ? AmmoInfoLayer.Get() : AmmonInfoLayer.Get();
    AmmoPresenter->Initialize(TB_CurrentAmmo, TB_ReserveAmmo, nullptr, InfoLayer);
}

void ULastFPSHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!CrosshairPresenter)
    {
        CrosshairPresenter = NewObject<ULastFPSEasyCrosshairPresenter>(this);
    }

    InitializePresenters();

    // 매 프레임 갱신은 NativeTick이 담당한다. 초기화만 여기서 시도하고, 아직 폰/PlayerState가
    // 준비되지 않았으면 짧은 주기로 재시도한다.
    if (UWorld* World = GetWorld())
    {
        if (!InitializeHUD())
        {
            World->GetTimerManager().SetTimer(
                RetryTimerHandle, this, &ULastFPSHUDWidget::RetryInitialize, 0.1f, true);
        }
    }
}

void ULastFPSHUDWidget::NativeDestruct()
{
    if (ALastFPSPlayerState* PlayerState = BoundPlayerState.Get())
    {
        PlayerState->OnDamageDealt.RemoveDynamic(this, &ULastFPSHUDWidget::HandleDamageDealt);
    }
    BoundPlayerState.Reset();

    if (UWeaponComponent* Weapon = BoundWeaponComponent.Get())
    {
        Weapon->OnWeaponEquippedChanged.RemoveDynamic(this, &ULastFPSHUDWidget::HandleWeaponEquippedChanged);
        Weapon->OnWeaponReloadStarted.RemoveDynamic(this, &ULastFPSHUDWidget::HandleReloadStarted);
        Weapon->OnWeaponReloadFinished.RemoveDynamic(this, &ULastFPSHUDWidget::HandleReloadFinished);
    }
    BoundWeaponComponent.Reset();

    if (ULastFPSGrapplingTargetingComponent* Targeting =
        BoundGrapplingTargetingComponent.Get())
    {
        Targeting->OnTargetAvailabilityChanged.RemoveDynamic(
            this,
            &ULastFPSHUDWidget::HandleGrapplingTargetAvailabilityChanged);
    }
    BoundGrapplingTargetingComponent.Reset();
    bPawnComponentsBound = false;

    if (ObjectiveHudPresenter)
    {
        ObjectiveHudPresenter->Unbind();
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RetryTimerHandle);
    }

    if (CombatFeedbackPresenter)
    {
        CombatFeedbackPresenter->Shutdown();
    }
    if (EnemyHealthPresenter)
    {
        EnemyHealthPresenter->Shutdown();
    }
    if (GrapplingReticlePresenter)
    {
        GrapplingReticlePresenter->Reset();
    }
    RemoveEasyCrosshair();
    if (WBP_StatusEffectList)
    {
        WBP_StatusEffectList->UninitializeFromAbilitySystem();
    }

    Super::NativeDestruct();
}

TOptional<FUIInputConfig> ULastFPSHUDWidget::GetDesiredInputConfig() const
{
    // HUD 는 입력 config 를 주장하지 않는다(empty). 커서/입력의 단일 소유는
    // PlayerController(ApplyInputConfigForMenuState). HUD 가 Game config 를 주장하면
    // 메뉴와 경쟁해 커서 desync 가 난다.
    return TOptional<FUIInputConfig>();
}

void ULastFPSHUDWidget::RetryInitialize()
{
    if (InitializeHUD() && SkillCooldownPresenter && SkillCooldownPresenter->IsInitialized())
    {
        GetWorld()->GetTimerManager().ClearTimer(RetryTimerHandle);
    }
}

void ULastFPSHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    TryBindPawnComponents();

    UWorld* World = GetWorld();
    if (!World) return;

    // InDeltaTime은 엔진이 넘겨주는 정확한 프레임 델타다. 별도 누적·보정이 필요 없다.
    const float DeltaTime = InDeltaTime;

    UAbilitySystemComponent* ASC = CachedASC.Get();
    if (SkillCooldownPresenter && SkillCooldownPresenter->IsInitialized())
    {
        SkillCooldownPresenter->Tick(ASC);
    }

    if (VitalsGaugePresenter)
    {
        VitalsGaugePresenter->Tick(DeltaTime);
    }
    if (ReloadPresenter)
    {
        ReloadPresenter->Tick(DeltaTime);
    }

    APlayerController* OwningPlayer = GetOwningPlayer();
    if (CombatFeedbackPresenter)
    {
        CombatFeedbackPresenter->Tick(OwningPlayer, DeltaTime);
    }
    if (GrapplingReticlePresenter)
    {
        GrapplingReticlePresenter->Tick(DeltaTime);
    }
    if (EnemyHealthPresenter)
    {
        EnemyHealthPresenter->Tick(DeltaTime);
    }
    if (ObjectiveHudPresenter)
    {
        // 월드가 늦게 준비되는 경우가 있어 매 틱 구독을 확인한다(중복 구독은 무시된다).
        ObjectiveHudPresenter->BindToWorld(World);
        ObjectiveHudPresenter->Tick(DeltaTime);
    }
    if (WBP_StatusEffectList)
    {
        WBP_StatusEffectList->UpdateRuntimeStates();
    }
}

void ULastFPSHUDWidget::TryBindPawnComponents()
{
    if (bPawnComponentsBound)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    ALastFPSHero* Hero = Cast<ALastFPSHero>(PC->GetPawn());
    if (!Hero)
    {
        return;
    }

    UWeaponComponent* Weapon = Hero->GetWeaponComponent();
    if (!Weapon)
    {
        return;
    }

    ULastFPSGrapplingTargetingComponent* GrapplingTargeting =
        Hero->GetGrapplingTargetingComponent();
    if (!GrapplingTargeting)
    {
        return;
    }

    Weapon->OnWeaponEquippedChanged.AddUniqueDynamic(this, &ULastFPSHUDWidget::HandleWeaponEquippedChanged);
    Weapon->OnWeaponReloadStarted.AddUniqueDynamic(this, &ULastFPSHUDWidget::HandleReloadStarted);
    Weapon->OnWeaponReloadFinished.AddUniqueDynamic(this, &ULastFPSHUDWidget::HandleReloadFinished);
    
    if (ReloadPresenter)
    {
        // 최초 바인딩 시에는 리로드 중이 아니므로 표시 요소를 숨겨 초기 상태를 정리한다.
        ReloadPresenter->SetVisible(false);
    }

    if (AmmoPresenter)
    {
        AmmoPresenter->BindToWeaponComponent(Weapon);
    }
    
    GrapplingTargeting->OnTargetAvailabilityChanged.AddUniqueDynamic(
        this,
        &ULastFPSHUDWidget::HandleGrapplingTargetAvailabilityChanged);

    BoundWeaponComponent = Weapon;
    BoundGrapplingTargetingComponent = GrapplingTargeting;
    bPawnComponentsBound = true;
    if (GrapplingReticlePresenter)
    {
        GrapplingReticlePresenter->SetAvailability(GrapplingTargeting->IsTargetAvailable());
    }
    RefreshEasyCrosshair();
}

bool ULastFPSHUDWidget::InitializeHUD()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PC->IsLocalController())
    {
        return false;
    }

    ALastFPSPlayerState* PS = PC->GetPlayerState<ALastFPSPlayerState>();
    if (!PS)
    {
        return false;
    }

    if (BoundPlayerState.Get() != PS)
    {
        if (ALastFPSPlayerState* PreviousPlayerState = BoundPlayerState.Get())
        {
            PreviousPlayerState->OnDamageDealt.RemoveDynamic(this, &ULastFPSHUDWidget::HandleDamageDealt);
        }

        PS->OnDamageDealt.AddUniqueDynamic(this, &ULastFPSHUDWidget::HandleDamageDealt);
        BoundPlayerState = PS;
    }

    UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
    const ULastFPSAttributeSet* AS = PS->GetAttributeSet();
    if (!ASC || !AS)
    {
        return false;
    }

    if (VitalsGaugePresenter)
    {
        VitalsGaugePresenter->BindToAbilitySystem(ASC, AS);
    }

    CachedASC = ASC;
    if (WBP_StatusEffectList)
    {
        WBP_StatusEffectList->InitializeWithAbilitySystem(ASC);
    }
    TryBindPawnComponents();

    const bool bSkillReady = SkillCooldownPresenter && SkillCooldownPresenter->TryInitialize(PC, ASC);
    if (bSkillReady)
    {
        // 초기화 직후 현재 쿨다운 상태를 즉시 반영해 첫 프레임 공백을 없앤다.
        SkillCooldownPresenter->Tick(ASC);
    }
    return bSkillReady;
}

void ULastFPSHUDWidget::HandleDamageDealt(
    float DamageAmount,
    float TotalDamageDealt,
    FVector DamageWorldLocation,
    AActor* DamageTargetActor,
    bool bCriticalHit)
{
    APlayerController* OwningPlayer = GetOwningPlayer();
    if (CombatFeedbackPresenter)
    {
        CombatFeedbackPresenter->SpawnDamageNumber(
            OwningPlayer, DamageAmount, TotalDamageDealt, DamageWorldLocation, DamageTargetActor, bCriticalHit);
    }
    if (EnemyHealthPresenter)
    {
        EnemyHealthPresenter->HandleDamage(OwningPlayer, DamageTargetActor, DamageAmount);
    }
    
    ShowHitMarker();
}

void ULastFPSHUDWidget::ShowDamageDirection(const FVector& DamageSourceDirection)
{
    if (CombatFeedbackPresenter)
    {
        CombatFeedbackPresenter->ShowDamageDirection(GetOwningPlayer(), DamageSourceDirection);
    }
}

UOverlay* ULastFPSHUDWidget::ResolveCrosshairHost()
{
    if (CrosshairHost)
    {
        return CrosshairHost;
    }

    if (!WidgetTree)
    {
        return nullptr;
    }

    UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
    if (!RootPanel)
    {
        UE_LOG(
            LogLastFPSHUDWidget,
            Error,
            TEXT("HUD '%s'에 EasyCrosshair를 배치할 수 있는 루트 패널이 없습니다."),
            *GetNameSafe(this));
        return nullptr;
    }

    CrosshairHost = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RuntimeCrosshairHost"));
    if (!CrosshairHost)
    {
        UE_LOG(
            LogLastFPSHUDWidget,
            Error,
            TEXT("HUD '%s'에서 EasyCrosshair 호스트 생성을 실패했습니다."),
            *GetNameSafe(this));
        return nullptr;
    }

    UPanelSlot* HostSlot = RootPanel->AddChild(CrosshairHost);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(HostSlot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        CanvasSlot->SetOffsets(FMargin(0.f));
    }
    else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(HostSlot))
    {
        OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
        OverlaySlot->SetVerticalAlignment(VAlign_Fill);
    }

    CrosshairHost->SetVisibility(ESlateVisibility::HitTestInvisible);
    return CrosshairHost;
}

void ULastFPSHUDWidget::RefreshEasyCrosshair()
{
    const UWeaponComponent* Weapon = BoundWeaponComponent.Get();
    if (!Weapon || !Weapon->HasWeapon())
    {
        SetEasyCrosshairVisibility(false);
        return;
    }

    TSoftObjectPtr<UecsCrosshairEditorAsset> CrosshairAssetReference = DefaultCrosshairAsset;
    FName FireAnimationName = DefaultCrosshairFireAnimationName;
    float FireAnimationDuration = DefaultCrosshairFireAnimationDuration;

    if (const ULastFPSWeaponDefinition* WeaponDefinition = Weapon->GetWeaponDefinition())
    {
        const FLastFPSWeaponCrosshairSettings& CrosshairSettings = WeaponDefinition->Crosshair;
        if (!CrosshairSettings.CrosshairAsset.IsNull())
        {
            CrosshairAssetReference = CrosshairSettings.CrosshairAsset;
            if (!CrosshairSettings.FireAnimationName.IsNone())
            {
                FireAnimationName = CrosshairSettings.FireAnimationName;
            }
            FireAnimationDuration = CrosshairSettings.FireAnimationDuration;
        }
    }

    if (CrosshairAssetReference.IsNull())
    {
        RemoveEasyCrosshair();
        if (!bCrosshairConfigurationWarningLogged)
        {
            UE_LOG(
                LogLastFPSHUDWidget,
                Warning,
                TEXT("HUD '%s'에 사용할 EasyCrosshair 에셋이 없습니다. 무기 Definition 또는 HUD 기본 설정을 확인하세요."),
                *GetNameSafe(this));
            bCrosshairConfigurationWarningLogged = true;
        }
        return;
    }

    UecsCrosshairEditorAsset* CrosshairAsset = CrosshairAssetReference.LoadSynchronous();
    if (!CrosshairAsset)
    {
        RemoveEasyCrosshair();
        if (!bCrosshairConfigurationWarningLogged)
        {
            UE_LOG(
                LogLastFPSHUDWidget,
                Error,
                TEXT("HUD '%s'에서 EasyCrosshair 에셋 '%s' 로드를 실패했습니다."),
                *GetNameSafe(this),
                *CrosshairAssetReference.ToString());
            bCrosshairConfigurationWarningLogged = true;
        }
        return;
    }

    UWorld* World = GetWorld();
    if (!World || !CrosshairPresenter)
    {
        UE_LOG(
            LogLastFPSHUDWidget,
            Error,
            TEXT("HUD '%s'에서 EasyCrosshair Presenter를 초기화하지 못했습니다."),
            *GetNameSafe(this));
        return;
    }

    if (!CrosshairPresenter->ShowCrosshair(
            *World,
            *CrosshairAsset,
            ResolveCrosshairHost(),
            FireAnimationName,
            FireAnimationDuration))
    {
        UE_LOG(
            LogLastFPSHUDWidget,
            Error,
            TEXT("HUD '%s'에 EasyCrosshair 에셋 '%s' 적용을 실패했습니다."),
            *GetNameSafe(this),
            *GetNameSafe(CrosshairAsset));
        return;
    }

    bCrosshairConfigurationWarningLogged = false;
}

void ULastFPSHUDWidget::RemoveEasyCrosshair()
{
    if (CrosshairPresenter)
    {
        CrosshairPresenter->Shutdown();
    }
}

void ULastFPSHUDWidget::SetEasyCrosshairVisibility(const bool bVisible)
{
    if (CrosshairPresenter)
    {
        CrosshairPresenter->SetVisible(bVisible);
    }
}

void ULastFPSHUDWidget::PlayCrosshairFireAnimation()
{
    if (CrosshairPresenter)
    {
        CrosshairPresenter->PlayFireAnimation();
    }
}

void ULastFPSHUDWidget::HandleWeaponEquippedChanged(bool bEquipped)
{
    if (bEquipped)
    {
        RefreshEasyCrosshair();
    }
    else
    {
        SetEasyCrosshairVisibility(false);
    }
}

void ULastFPSHUDWidget::HandleReloadStarted(float ReloadDuration)
{
    if (AmmoTwinkleAnim)
    {
        PlayAnimation(AmmoTwinkleAnim);
    }
    
    if (ReloadPresenter.Get())
    {
        ReloadPresenter.Get()->SetReloadStarted(ReloadDuration);
    }
    
    SetEasyCrosshairVisibility(false);
}

void ULastFPSHUDWidget::HandleReloadFinished(bool bCompleted)
{
    SetEasyCrosshairVisibility(true);
    
    if (AmmoTwinkleAnim)
    {
        StopAnimation(AmmoTwinkleAnim);
    }
    
    if (ReloadPresenter.Get())
    {
        ReloadPresenter.Get()->SetReloadFinished(bCompleted);
    }
}

void ULastFPSHUDWidget::HandleGrapplingTargetAvailabilityChanged(
    const bool bTargetAvailable)
{
    if (GrapplingReticlePresenter)
    {
        GrapplingReticlePresenter->SetAvailability(bTargetAvailable);
    }
}

void ULastFPSHUDWidget::ShowHitMarker()
{
    if (CombatFeedbackPresenter)
    {
        CombatFeedbackPresenter->ShowHitMarker();
    }
}
