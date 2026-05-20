#include "UI/LastFPSHUDWidget.h"
#include "UI/LastFPSSkillCooldownSlotWidget.h"
#include "UI/LastFPSHUDStyle.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_Skill1Cooldown.h"
#include "AbilitySystem/Effects/GE_Skill2Cooldown.h"
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

#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Game/LastFPSMatchGameState.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_Skill1, "Cooldown.Skill1");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_Skill2, "Cooldown.Skill2");

ULastFPSHUDWidget::ULastFPSHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    GaugeBackgroundColor     = LastFPSHUDStyle::GaugeBackground();
    HealthFillColor          = LastFPSHUDStyle::HealthFill();
    HealthLowFillColor       = LastFPSHUDStyle::HealthLowFill();
    StaminaFillColor         = LastFPSHUDStyle::StaminaFill();
    StaminaLowFillColor      = LastFPSHUDStyle::StaminaLowFill();
    UltimateFillColor        = LastFPSHUDStyle::UltimateFill();
    UltimateReadyFillColor   = LastFPSHUDStyle::UltimateReady();
    HeatFillColor            = LastFPSHUDStyle::HeatFill();
    HeatOverheatedFillColor  = LastFPSHUDStyle::HeatOverheated();
    KillFeedKillerColor      = LastFPSHUDStyle::KillFeedKiller();
    KillFeedVictimColor      = LastFPSHUDStyle::KillFeedVictim();
    KillFeedSeparatorColor   = LastFPSHUDStyle::KillFeedSeparator();
    KillFeedLocalPlayerColor = LastFPSHUDStyle::KillFeedLocalPlayer();
}

void ULastFPSHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ApplyGaugeBarBackground(PB_Health);
    ApplyGaugeBarBackground(PB_Stamina);
    ApplyGaugeBarBackground(PB_Ultimate);
    ApplyGaugeBarBackground(PB_Heat);

    if (HitMarkerImage)
    {
        HitMarkerImage->SetVisibility(ESlateVisibility::Collapsed);
    }

    TryBindKillFeed();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            HUDRefreshTimerHandle,
            this,
            &ULastFPSHUDWidget::HUDRefreshTickFromTimer,
            0.033f,
            true);

        if (!InitializeHUD())
        {
            World->GetTimerManager().SetTimer(
                RetryTimerHandle, this, &ULastFPSHUDWidget::RetryInitialize, 0.1f, true);
        }
    }
}

void ULastFPSHUDWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HUDRefreshTimerHandle);
        World->GetTimerManager().ClearTimer(RetryTimerHandle);
    }

    if (ALastFPSMatchGameState* MGS = BoundMatchGameState.Get())
    {
        MGS->OnKillFeed.Remove(KillFeedHandle);
    }
    BoundMatchGameState.Reset();
    KillFeedHandle.Reset();
    KillFeedLines.Empty();

    Super::NativeDestruct();
}

void ULastFPSHUDWidget::RetryInitialize()
{
    if (InitializeHUD() && bSkillSlotsInitialized)
    {
        GetWorld()->GetTimerManager().ClearTimer(RetryTimerHandle);
    }
}

void ULastFPSHUDWidget::TryBindKillFeed()
{
    if (BoundMatchGameState.IsValid())
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    ALastFPSMatchGameState* MGS = World->GetGameState<ALastFPSMatchGameState>();
    if (!MGS)
        return;

    BoundMatchGameState = MGS;
    KillFeedHandle      = MGS->OnKillFeed.AddUObject(this, &ULastFPSHUDWidget::HandleKillFeed);
}

void ULastFPSHUDWidget::HandleKillFeed(const FString& KillerName, const FString& VictimName)
{
    AddKillFeedLine(KillerName, VictimName);
    OnKillFeedEntry(KillerName, VictimName);
}

FString ULastFPSHUDWidget::GetLocalKillFeedDisplayName() const
{
    const APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return FString();
    }

    if (const ALastFPSCharacterBase* Character = Cast<ALastFPSCharacterBase>(PC->GetPawn()))
    {
        return Character->GetKillFeedDisplayName();
    }

    if (const APlayerState* PS = PC->GetPlayerState<APlayerState>())
    {
        return PS->GetPlayerName();
    }

    return FString();
}

UTextBlock* ULastFPSHUDWidget::CreateKillFeedText(const FString& Text, const FLinearColor& Color)
{
    UTextBlock* TextBlock = NewObject<UTextBlock>(this);
    if (!TextBlock)
    {
        return nullptr;
    }

    TextBlock->SetText(FText::FromString(Text));
    TextBlock->SetColorAndOpacity(FSlateColor(Color));
    TextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
    TextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));
    return TextBlock;
}

void ULastFPSHUDWidget::AddKillFeedLine(const FString& KillerName, const FString& VictimName)
{
    if (!KillFeedContainer)
    {
        return;
    }

    const FString LocalName = GetLocalKillFeedDisplayName();
    const bool bLocalIsKiller = !LocalName.IsEmpty()
        && KillerName.Equals(LocalName, ESearchCase::IgnoreCase);
    const bool bLocalIsVictim = !LocalName.IsEmpty()
        && VictimName.Equals(LocalName, ESearchCase::IgnoreCase);

    const FLinearColor KillerColor = bLocalIsKiller ? KillFeedLocalPlayerColor : KillFeedKillerColor;
    const FLinearColor VictimColor = bLocalIsVictim ? KillFeedLocalPlayerColor : KillFeedVictimColor;

    UHorizontalBox* Row = NewObject<UHorizontalBox>(this);
    if (!Row)
    {
        return;
    }

    if (UTextBlock* KillerText = CreateKillFeedText(KillerName, KillerColor))
    {
        Row->AddChildToHorizontalBox(KillerText);
    }

    if (UTextBlock* SeparatorText = CreateKillFeedText(TEXT("  \u2192  "), KillFeedSeparatorColor))
    {
        Row->AddChildToHorizontalBox(SeparatorText);
    }

    if (UTextBlock* VictimText = CreateKillFeedText(VictimName, VictimColor))
    {
        Row->AddChildToHorizontalBox(VictimText);
    }

    if (UVerticalBox* VBox = Cast<UVerticalBox>(KillFeedContainer))
    {
        VBox->AddChildToVerticalBox(Row);
    }
    else
    {
        KillFeedContainer->AddChild(Row);
    }

    KillFeedLines.Add(Row);

    while (KillFeedLines.Num() > MaxKillFeedEntries)
    {
        if (UWidget* Oldest = KillFeedLines[0])
        {
            Oldest->RemoveFromParent();
        }
        KillFeedLines.RemoveAt(0);
    }
}

void ULastFPSHUDWidget::HUDRefreshTickFromTimer()
{
    const UWorld* World = GetWorld();
    const float DeltaTime = World ? World->GetDeltaSeconds() : 0.033f;
    HUDRefreshTick(DeltaTime);
}

void ULastFPSHUDWidget::HUDRefreshTick(const float DeltaTime)
{
    TryBindPawnComponents();

    if (!bSkillSlotsInitialized)
    {
        TryInitSkillSlots();
    }

    if (bSkillSlotsInitialized)
    {
        TickSkillSlots();
    }

    TickSmoothedGauges(DeltaTime);

    if (!MatchTimerText)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const ALastFPSMatchGameState* MGS = World->GetGameState<ALastFPSMatchGameState>();
    if (!MGS)
    {
        return;
    }

    const int32 TotalSec = FMath::Max(0, FMath::CeilToInt(MGS->GetMatchTimeRemaining()));
    if (TotalSec == CachedMatchTimeIntSec)
    {
        return;
    }

    CachedMatchTimeIntSec = TotalSec;

    const int32 Min = TotalSec / 60;
    const int32 Sec = TotalSec % 60;
    MatchTimerText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Min, Sec)));
}

bool ULastFPSHUDWidget::TryInitSkillSlots()
{
    if (bSkillSlotsInitialized)
    {
        return true;
    }

    if (!WBP_SkillCooldownSlot_Q || !WBP_SkillCooldownSlot_E || !WBP_SkillCooldownSlot_F || !CachedASC.IsValid())
    {
        return false;
    }

    WBP_SkillCooldownSlot_Q->ConfigureCooldownSlot(
        TAG_Cooldown_Skill1, ULastFPSGE_Skill1Cooldown::StaticClass());
    WBP_SkillCooldownSlot_Q->SetKeyLabel(FText::FromString(TEXT("Q")));

    WBP_SkillCooldownSlot_E->ConfigureCooldownSlot(
        TAG_Cooldown_Skill2, ULastFPSGE_Skill2Cooldown::StaticClass());
    WBP_SkillCooldownSlot_E->SetKeyLabel(FText::FromString(TEXT("E")));

    WBP_SkillCooldownSlot_F->ConfigureUltimateSlot();
    WBP_SkillCooldownSlot_F->SetKeyLabel(FText::FromString(TEXT("F")));

    TickSkillSlots();
    bSkillSlotsInitialized = true;
    return true;
}

void ULastFPSHUDWidget::TickSkillSlots()
{
    if (!bSkillSlotsInitialized || !CachedASC.IsValid())
    {
        return;
    }

    UAbilitySystemComponent* ASC = CachedASC.Get();
    const ULastFPSAttributeSet* AS = nullptr;
    if (const APlayerController* PC = GetOwningPlayer())
    {
        if (const ALastFPSPlayerState* PS = PC->GetPlayerState<ALastFPSPlayerState>())
        {
            AS = PS->GetAttributeSet();
        }
    }

    if (WBP_SkillCooldownSlot_Q) { WBP_SkillCooldownSlot_Q->UpdateFromASC(ASC); }
    if (WBP_SkillCooldownSlot_E) { WBP_SkillCooldownSlot_E->UpdateFromASC(ASC); }
    if (WBP_SkillCooldownSlot_F) { WBP_SkillCooldownSlot_F->UpdateFromASC(ASC, AS); }
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

    Weapon->OnHeatChanged.AddDynamic(this, &ULastFPSHUDWidget::HandleHeatChanged);
    CachedMaxHeat        = Weapon->GetMaxHeat();
    CachedHeatOverheated = Weapon->IsOverheated();
    HeatGauge.Initialize(Weapon->GetCurrentHeat(), CachedMaxHeat);
    BroadcastHeatDisplay();

    Weapon->OnWeaponEquippedChanged.AddDynamic(this, &ULastFPSHUDWidget::HandleWeaponEquippedChanged);
    OnCrosshairVisibilityChanged(Weapon->HasWeapon());

    bPawnComponentsBound = true;
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

    UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
    const ULastFPSAttributeSet* AS = PS->GetAttributeSet();
    if (!ASC || !AS)
    {
        return false;
    }

    if (!bAttributeDelegatesBound)
    {
        ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
            .AddUObject(this, &ULastFPSHUDWidget::HandleHealthChanged);
        ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetStaminaAttribute())
            .AddUObject(this, &ULastFPSHUDWidget::HandleStaminaChanged);
        ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetUltimateGaugeAttribute())
            .AddUObject(this, &ULastFPSHUDWidget::HandleUltimateGaugeChanged);
        bAttributeDelegatesBound = true;
    }

    HealthGauge.Initialize(AS->GetHealth(), AS->GetMaxHealth());
    StaminaGauge.Initialize(AS->GetStamina(), AS->GetMaxStamina());
    UltimateGauge.Initialize(AS->GetUltimateGauge(), AS->GetMaxUltimateGauge());
    BroadcastHealthDisplay();
    BroadcastStaminaDisplay();
    BroadcastUltimateGaugeDisplay();

    CachedASC = ASC;
    TryBindPawnComponents();
    return TryInitSkillSlots();
}

void ULastFPSHUDWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
    HealthGauge.SetTarget(Data.NewValue);
    if (!HealthGauge.bInterpActive)
    {
        BroadcastHealthDisplay();
    }
}

void ULastFPSHUDWidget::HandleStaminaChanged(const FOnAttributeChangeData& Data)
{
    StaminaGauge.SetTarget(Data.NewValue);
    if (!StaminaGauge.bInterpActive)
    {
        BroadcastStaminaDisplay();
    }
}

void ULastFPSHUDWidget::HandleUltimateGaugeChanged(const FOnAttributeChangeData& Data)
{
    UltimateGauge.SetTarget(Data.NewValue);
    if (!UltimateGauge.bInterpActive)
    {
        BroadcastUltimateGaugeDisplay();
    }

    if (bSkillSlotsInitialized && WBP_SkillCooldownSlot_F && CachedASC.IsValid())
    {
        const ULastFPSAttributeSet* AS = nullptr;
        if (const APlayerController* LocalPC = GetOwningPlayer())
        {
            if (const ALastFPSPlayerState* LocalPS = LocalPC->GetPlayerState<ALastFPSPlayerState>())
            {
                AS = LocalPS->GetAttributeSet();
            }
        }
        WBP_SkillCooldownSlot_F->UpdateFromASC(CachedASC.Get(), AS);
    }
}

void ULastFPSHUDWidget::TickSmoothedGauges(float DeltaTime)
{
    if (HealthGauge.Tick(DeltaTime, GaugeFillDuration))
    {
        BroadcastHealthDisplay();
    }

    if (StaminaGauge.Tick(DeltaTime, GaugeFillDuration))
    {
        BroadcastStaminaDisplay();
    }

    if (UltimateGauge.Tick(DeltaTime, GaugeFillDuration))
    {
        BroadcastUltimateGaugeDisplay();
    }

    if (HeatGauge.Tick(DeltaTime, GaugeFillDuration))
    {
        BroadcastHeatDisplay();
    }
}

bool ULastFPSHUDWidget::IsLowResource(float Current, float Max) const
{
    if (Max <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    return (Current / Max) < LowResourceThreshold;
}

FLinearColor ULastFPSHUDWidget::ResolveHealthFillColor() const
{
    return IsLowResource(HealthGauge.Displayed, HealthGauge.Max)
        ? HealthLowFillColor
        : HealthFillColor;
}

FLinearColor ULastFPSHUDWidget::ResolveStaminaFillColor() const
{
    return IsLowResource(StaminaGauge.Displayed, StaminaGauge.Max)
        ? StaminaLowFillColor
        : StaminaFillColor;
}

FLinearColor ULastFPSHUDWidget::ResolveUltimateFillColor() const
{
    if (UltimateGauge.Max > KINDA_SMALL_NUMBER
        && UltimateGauge.Displayed >= UltimateGauge.Max - KINDA_SMALL_NUMBER)
    {
        return UltimateReadyFillColor;
    }

    return UltimateFillColor;
}

FLinearColor ULastFPSHUDWidget::ResolveHeatFillColor() const
{
    return CachedHeatOverheated ? HeatOverheatedFillColor : HeatFillColor;
}

void ULastFPSHUDWidget::ApplyGaugeBarBackground(UProgressBar* Bar) const
{
    if (!Bar)
    {
        return;
    }

    FProgressBarStyle Style = Bar->GetWidgetStyle();
    Style.BackgroundImage.TintColor = FSlateColor(GaugeBackgroundColor);
    Bar->SetWidgetStyle(Style);
}

void ULastFPSHUDWidget::ApplyGaugeBar(
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

void ULastFPSHUDWidget::BroadcastHealthDisplay()
{
    ApplyGaugeBar(PB_Health, HealthGauge.Displayed, HealthGauge.Max, ResolveHealthFillColor());
    OnHealthChanged(HealthGauge.Displayed, HealthGauge.Max);
}

void ULastFPSHUDWidget::BroadcastStaminaDisplay()
{
    ApplyGaugeBar(PB_Stamina, StaminaGauge.Displayed, StaminaGauge.Max, ResolveStaminaFillColor());
    OnStaminaChanged(StaminaGauge.Displayed, StaminaGauge.Max);
}

void ULastFPSHUDWidget::BroadcastUltimateGaugeDisplay()
{
    ApplyGaugeBar(PB_Ultimate, UltimateGauge.Displayed, UltimateGauge.Max, ResolveUltimateFillColor());
    OnUltimateGaugeChanged(UltimateGauge.Displayed, UltimateGauge.Max);
}

void ULastFPSHUDWidget::BroadcastHeatDisplay()
{
    ApplyGaugeBar(PB_Heat, HeatGauge.Displayed, CachedMaxHeat, ResolveHeatFillColor());
    OnHeatChanged(HeatGauge.Displayed, CachedMaxHeat, CachedHeatOverheated);
}

void ULastFPSHUDWidget::HandleHeatChanged(float Current, float Max, bool bIsOverheated)
{
    const bool bOverheatedChanged = (bIsOverheated != CachedHeatOverheated);

    CachedMaxHeat        = Max;
    CachedHeatOverheated = bIsOverheated;
    HeatGauge.Max        = FMath::Max(Max, KINDA_SMALL_NUMBER);
    HeatGauge.SetTarget(Current);

    if (!HeatGauge.bInterpActive || bOverheatedChanged)
    {
        // 오버히트 색상은 즉시 반영, heat fill만 보간
        BroadcastHeatDisplay();
    }
}

void ULastFPSHUDWidget::HandleWeaponEquippedChanged(bool bEquipped)
{
    OnCrosshairVisibilityChanged(bEquipped);
}

void ULastFPSHUDWidget::ShowHitMarker()
{
    if (!HitMarkerImage)
        return;

    HitMarkerImage->SetVisibility(ESlateVisibility::HitTestInvisible);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HitMarkerTimerHandle);
        World->GetTimerManager().SetTimer(
            HitMarkerTimerHandle,
            FTimerDelegate::CreateUObject(this, &ULastFPSHUDWidget::HideHitMarker),
            HitMarkerDisplayDuration,
            false);
    }
}

void ULastFPSHUDWidget::HideHitMarker()
{
    if (HitMarkerImage)
        HitMarkerImage->SetVisibility(ESlateVisibility::Collapsed);
}
