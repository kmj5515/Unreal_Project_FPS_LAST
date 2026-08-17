
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
#include "UI/HUD/Presenters/LastFPSWeaponSlotPresenter.h"
#include "UI/HUD/Presenters/LastFPSObjectiveHudPresenter.h"
#include "UI/HUD/LastFPSCaptureObjectiveWidget.h"
#include "UI/HUD/LastFPSDefendObjectiveWidget.h"
#include "UI/LastFPSDamageNumberWidget.h"
#include "UI/HUD/LastFPSStatusEffectListWidget.h"
#include "UI/HUD/Quest/LastFPSQuestTrackerWidget.h"
#include "UI/HUD/Quest/LastFPSObjectiveMarkerWidget.h"
#include "UI/HUD/LastFPSHUDStyle.h"
#include "UI/Framework/LastFPSPrimaryGameLayout.h"
#include "Cinematics/LastFPSCinematicPlaybackSubsystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerController.h"
#include "PrimaryGameLayout.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "Game/LastFPSGameStateBase.h"
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

    // 무전 자막은 컷신 대사를 실어 나르므로 기본으로 유지한다. WBP 에서 목록을 바꿀 수 있다.
    CinematicPersistentWidgets.Add(TEXT("WBP_RadioTransmission"));
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
    UOverlay* InfoLayer = AmmoInfoLayer.Get();
    AmmoPresenter->Initialize(TB_CurrentAmmo, TB_ReserveAmmo, nullptr, InfoLayer);

    WeaponSlotPresenter = NewObject<ULastFPSWeaponSlotPresenter>(this);
    WeaponSlotPresenter->Initialize(WeaponSlotContainer, WeaponSlotWidgetClass);
}

void ULastFPSHUDWidget::ApplyCombatHUDVisibility()
{
    if (!BottomLayer)
    {
        return;
    }

    // "어떤 맵에서 전투 UI 를 띄우는가"는 맵별 규칙이라 GameMode 가 소유한다.
    // HUD 가 레벨 이름이나 태그로 직접 판단하면 맵이 늘 때마다 HUD 를 고쳐야 한다.
    // 다만 GameMode 는 서버에만 존재하므로 복제된 GameState 값을 읽는다.
    const ALastFPSGameStateBase* GameState = GetMapUIRulesOwner();

    // 규칙이 아직 도착하지 않았으면 숨기지 않는다. 전투 중에 UI 가 사라지는 쪽이 더 나쁘다.
    const bool bShowCombat =
        !GameState || !GameState->HasMapUIRules() || GameState->GetMapUIRules().bShowCombatHUD;

    BottomLayer->SetVisibility(
        bShowCombat ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void ULastFPSHUDWidget::ApplyQuestHUDVisibility()
{
    if (!WBP_QuestTracker && !WBP_ObjectiveMarkers)
    {
        return;
    }

    // 전투 HUD 와 같은 이유로 맵별 규칙은 GameMode 가 소유하고, 값은 GameState 로 복제된다.
    // 기본값이 "숨김"이라 규칙이 도착하기 전에는 띄우지 않는다.
    // 퀘스트가 없는 맵(메인 메뉴 등)에 빈 패널이 뜨는 쪽이 더 나쁘기 때문이다.
    // 규칙이 늦게 도착하면 OnMapUIRulesChanged 구독이 이 함수를 다시 부른다.
    const ALastFPSGameStateBase* GameState = GetMapUIRulesOwner();
    const bool bShowQuestHUD =
        GameState && GameState->HasMapUIRules() && GameState->GetMapUIRules().bShowQuestTracker;

    const ESlateVisibility QuestHUDVisibility =
        bShowQuestHUD ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;

    if (WBP_QuestTracker && !bShowQuestHUD)
    {
        WBP_QuestTracker->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (WBP_ObjectiveMarkers)
    {
        WBP_ObjectiveMarkers->SetVisibility(QuestHUDVisibility);
    }
}

void ULastFPSHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ApplyCombatHUDVisibility();
    ApplyQuestHUDVisibility();
    BindMapUIRulesEvents();
    BindCinematicEvents();

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

const ALastFPSGameStateBase* ULastFPSHUDWidget::GetMapUIRulesOwner() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetGameState<ALastFPSGameStateBase>() : nullptr;
}

void ULastFPSHUDWidget::BindMapUIRulesEvents()
{
    const ALastFPSGameStateBase* GameState = GetMapUIRulesOwner();
    if (!GameState)
    {
        // 클라이언트에서는 GameState 복제가 이 위젯 생성보다 늦을 수 있다. 생길 때까지만 재시도한다.
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                MapUIRulesBindRetryTimerHandle,
                this,
                &ULastFPSHUDWidget::BindMapUIRulesEvents,
                0.1f,
                true);
        }
        return;
    }

    if (BoundMapUIRulesGameState.Get() == GameState)
    {
        return;
    }

    UnbindMapUIRulesEvents();
    BoundMapUIRulesGameState = GameState;
    MapUIRulesChangedHandle =
        const_cast<ALastFPSGameStateBase*>(GameState)->OnMapUIRulesChanged.AddUObject(
            this,
            &ULastFPSHUDWidget::HandleMapUIRulesChanged);

    // 구독 전에 이미 도착한 규칙이 있으면 지금 반영한다.
    HandleMapUIRulesChanged();
}

void ULastFPSHUDWidget::UnbindMapUIRulesEvents()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MapUIRulesBindRetryTimerHandle);
    }

    if (const ALastFPSGameStateBase* GameState = BoundMapUIRulesGameState.Get();
        GameState && MapUIRulesChangedHandle.IsValid())
    {
        const_cast<ALastFPSGameStateBase*>(GameState)->OnMapUIRulesChanged.Remove(
            MapUIRulesChangedHandle);
    }

    MapUIRulesChangedHandle.Reset();
    BoundMapUIRulesGameState.Reset();
}

void ULastFPSHUDWidget::HandleMapUIRulesChanged()
{
    ApplyCombatHUDVisibility();
    ApplyQuestHUDVisibility();
}

void ULastFPSHUDWidget::BindCinematicEvents()
{
    ULastFPSCinematicPlaybackSubsystem* Cinematics =
        ULastFPSCinematicPlaybackSubsystem::Get(this);
    if (!Cinematics || BoundCinematicSubsystem.Get() == Cinematics)
    {
        return;
    }

    UnbindCinematicEvents();

    Cinematics->OnCinematicStarted.AddDynamic(this, &ULastFPSHUDWidget::HandleCinematicStarted);
    Cinematics->OnCinematicFinished.AddDynamic(this, &ULastFPSHUDWidget::HandleCinematicFinished);
    BoundCinematicSubsystem = Cinematics;
}

void ULastFPSHUDWidget::UnbindCinematicEvents()
{
    if (ULastFPSCinematicPlaybackSubsystem* Cinematics = BoundCinematicSubsystem.Get())
    {
        Cinematics->OnCinematicStarted.RemoveDynamic(this, &ULastFPSHUDWidget::HandleCinematicStarted);
        Cinematics->OnCinematicFinished.RemoveDynamic(this, &ULastFPSHUDWidget::HandleCinematicFinished);
    }
    BoundCinematicSubsystem.Reset();
}

void ULastFPSHUDWidget::HandleCinematicStarted(bool /*bSkippable*/)
{
    SetCinematicUIHidden(true);
}

void ULastFPSHUDWidget::HandleCinematicFinished(bool /*bSkipped*/)
{
    SetCinematicUIHidden(false);
}

void ULastFPSHUDWidget::SetCinematicUIHidden(const bool bHidden)
{
    // 화면·팝업 레이어는 HUD 의 소유가 아니므로 레이아웃이 처리한다.
    if (ULastFPSPrimaryGameLayout* Layout =
        Cast<ULastFPSPrimaryGameLayout>(UPrimaryGameLayout::GetPrimaryGameLayout(GetOwningPlayer())))
    {
        Layout->SetLayersHiddenForCinematic(bHidden);
    }

    if (!bHidden)
    {
        for (const TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>& Entry : CinematicRestoreVisibilities)
        {
            if (UWidget* Child = Entry.Key.Get())
            {
                Child->SetVisibility(Entry.Value);
            }
        }
        CinematicRestoreVisibilities.Reset();
        return;
    }

    // 중복 진입 시 이미 접어 둔 상태를 "원래 상태"로 덮어써 복구가 깨지는 것을 막는다.
    if (!CinematicRestoreVisibilities.IsEmpty())
    {
        return;
    }

    UPanelWidget* Root = Cast<UPanelWidget>(GetRootWidget());
    if (!Root)
    {
        return;
    }

    for (int32 Index = 0; Index < Root->GetChildrenCount(); ++Index)
    {
        UWidget* Child = Root->GetChildAt(Index);
        if (!Child || CinematicPersistentWidgets.Contains(Child->GetFName()))
        {
            continue;
        }

        CinematicRestoreVisibilities.Add(Child, Child->GetVisibility());
        Child->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ULastFPSHUDWidget::NativeDestruct()
{
    UnbindMapUIRulesEvents();
    UnbindCinematicEvents();

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

    if (ALastFPSHero* Hero = BoundHero.Get())
    {
        Hero->OnAimingChanged.RemoveDynamic(this, &ULastFPSHUDWidget::HandleAimingChanged);
    }
    BoundHero.Reset();
    CancelScopeOverlayPreload();
    ClearScopeOverlay();
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
    if (WeaponSlotPresenter)
    {
        WeaponSlotPresenter->Reset();
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
    Hero->OnAimingChanged.AddUniqueDynamic(this, &ULastFPSHUDWidget::HandleAimingChanged);
    BoundHero = Hero;

    if (ReloadPresenter)
    {
        // 최초 바인딩 시에는 리로드 중이 아니므로 표시 요소를 숨겨 초기 상태를 정리한다.
        ReloadPresenter->SetVisible(false);
    }

    if (AmmoPresenter)
    {
        AmmoPresenter->BindToWeaponComponent(Weapon);
    }

    if (WeaponSlotPresenter)
    {
        WeaponSlotPresenter->BindToWeaponComponent(Weapon);
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
    RequestScopeOverlayPreload();
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

    ClearScopeOverlay();
    RequestScopeOverlayPreload();
    UpdateScopeOverlay(bEquipped && bLastAiming);
}

void ULastFPSHUDWidget::HandleAimingChanged(bool bIsAiming)
{
    bLastAiming = bIsAiming;
    UpdateScopeOverlay(bIsAiming);
}

void ULastFPSHUDWidget::UpdateScopeOverlay(bool bAiming)
{
    const UWeaponComponent* Weapon = BoundWeaponComponent.Get();
    const FLastFPSWeaponScopeSettings* ScopeSettings = Weapon ? Weapon->GetScopeSettings() : nullptr;
    const bool bWantOverlay = bAiming && ScopeSettings && !ScopeSettings->ScopeOverlayWidgetClass.IsNull();

    if (!bWantOverlay)
    {
        if (ScopeOverlayWidget)
        {
            ScopeOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    if (!ScopeOverlayHost)
    {
        UE_LOG(LogLastFPSHUDWidget, Warning,
            TEXT("HUD '%s'에 ScopeOverlayHost가 없어 스코프 오버레이를 표시할 수 없습니다. HUD 위젯 BP에 BindWidgetOptional 슬롯을 배치하세요."),
            *GetNameSafe(this));
        return;
    }

    UClass* OverlayClass = ScopeSettings->ScopeOverlayWidgetClass.Get();
    if (!OverlayClass)
    {
        OverlayClass = ScopeSettings->ScopeOverlayWidgetClass.LoadSynchronous();
    }

    if (!OverlayClass)
    {
        UE_LOG(LogLastFPSHUDWidget, Warning,
            TEXT("HUD '%s'에서 스코프 오버레이 위젯 클래스 로드에 실패했습니다. WeaponDefinition의 Scope.ScopeOverlayWidgetClass를 확인하세요."),
            *GetNameSafe(this));
        return;
    }

    if (ScopeOverlayWidget && ScopeOverlayWidgetClass != OverlayClass)
    {
        ClearScopeOverlay();
    }

    if (!ScopeOverlayWidget)
    {
        ScopeOverlayWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), OverlayClass);
        if (!ScopeOverlayWidget)
        {
            return;
        }
        ScopeOverlayWidgetClass = OverlayClass;

        if (UOverlaySlot* HostSlot = Cast<UOverlaySlot>(ScopeOverlayHost->AddChild(ScopeOverlayWidget)))
        {
            HostSlot->SetHorizontalAlignment(HAlign_Fill);
            HostSlot->SetVerticalAlignment(VAlign_Fill);
        }
    }

    ScopeOverlayWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULastFPSHUDWidget::ClearScopeOverlay()
{
    if (ScopeOverlayWidget)
    {
        ScopeOverlayWidget->RemoveFromParent();
        ScopeOverlayWidget = nullptr;
    }
    ScopeOverlayWidgetClass = nullptr;
}

void ULastFPSHUDWidget::RequestScopeOverlayPreload()
{
    const UWeaponComponent* Weapon = BoundWeaponComponent.Get();
    const FLastFPSWeaponScopeSettings* ScopeSettings = Weapon ? Weapon->GetScopeSettings() : nullptr;
    if (!ScopeSettings)
    {
        CancelScopeOverlayPreload();
        return;
    }

    const FSoftObjectPath OverlayPath = ScopeSettings->ScopeOverlayWidgetClass.ToSoftObjectPath();
    if (!OverlayPath.IsValid() || (ScopeOverlayLoadHandle.IsValid() && PendingScopeOverlayPath == OverlayPath))
    {
        return;
    }

    CancelScopeOverlayPreload();
    PendingScopeOverlayPath = OverlayPath;
    ScopeOverlayLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        OverlayPath,
        FStreamableDelegate::CreateUObject(this, &ULastFPSHUDWidget::HandleScopeOverlayLoaded),
        FStreamableManager::AsyncLoadHighPriority);

    if (!ScopeOverlayLoadHandle.IsValid())
    {
        UE_LOG(LogLastFPSHUDWidget, Error,
            TEXT("HUD '%s'에서 스코프 오버레이 '%s' 비동기 로드를 시작하지 못했습니다."),
            *GetNameSafe(this), *OverlayPath.ToString());
        PendingScopeOverlayPath.Reset();
    }
}

void ULastFPSHUDWidget::CancelScopeOverlayPreload()
{
    if (ScopeOverlayLoadHandle.IsValid())
    {
        ScopeOverlayLoadHandle->CancelHandle();
        ScopeOverlayLoadHandle.Reset();
    }
    PendingScopeOverlayPath.Reset();
}

void ULastFPSHUDWidget::HandleScopeOverlayLoaded()
{
    ScopeOverlayLoadHandle.Reset();
    PendingScopeOverlayPath.Reset();

    if (bLastAiming)
    {
        UpdateScopeOverlay(true);
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
