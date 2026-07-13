#include "UI/HUD/LastFPSHUDWidget.h"
#include "UI/LastFPSDamageNumberWidget.h"
#include "UI/HUD/LastFPSSkillCooldownSlotWidget.h"
#include "UI/HUD/LastFPSHUDStyle.h"
#include "AbilitySystemComponent.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Tables/LastFPSCharacterSkillData.h"
#include "Engine/GameInstance.h"
#include "Styling/SlateBrush.h"
#include "UI/Framework/LastFPSUITags.h"
#include "Utility/LastFPSTags.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Texture2D.h"
#include "PrimaryGameLayout.h"
#include "Skills/LastFPSSkillDataSubsystem.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

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
#include "Input/CommonUIInputTypes.h"
#include "Input/UIActionBindingHandle.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

ULastFPSHUDWidget::ULastFPSHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    GaugeBackgroundColor     = LastFPSHUDStyle::GaugeBackground();
    HealthFillColor          = LastFPSHUDStyle::HealthFill();
    HealthLowFillColor       = LastFPSHUDStyle::HealthLowFill();
    StaminaFillColor         = LastFPSHUDStyle::StaminaFill();
    StaminaLowFillColor      = LastFPSHUDStyle::StaminaLowFill();
    DamageNumberWidgetClass  = ULastFPSDamageNumberWidget::StaticClass();
}

void ULastFPSHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ApplyGaugeBarBackground(PB_Health);
    ApplyGaugeBarBackground(PB_Stamina);

    if (HitMarkerImage)
    {
        InitializeHitMarkerMaterial();
        SetHitMarkerSpread(0.f);
        HitMarkerImage->SetVisibility(ESlateVisibility::Collapsed);
    }

    InitializeCrosshairMaterial();
    SetCrosshairSpread(CrosshairBaseSpread);

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
    if (ALastFPSPlayerState* PlayerState = BoundPlayerState.Get())
    {
        PlayerState->OnDamageDealt.RemoveDynamic(this, &ULastFPSHUDWidget::HandleDamageDealt);
    }
    BoundPlayerState.Reset();

    if (UWeaponComponent* Weapon = BoundWeaponComponent.Get())
    {
        Weapon->OnWeaponEquippedChanged.RemoveDynamic(this, &ULastFPSHUDWidget::HandleWeaponEquippedChanged);
    }
    BoundWeaponComponent.Reset();
    bPawnComponentsBound = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HUDRefreshTimerHandle);
        World->GetTimerManager().ClearTimer(RetryTimerHandle);
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
    if (InitializeHUD() && bSkillSlotsInitialized)
    {
        GetWorld()->GetTimerManager().ClearTimer(RetryTimerHandle);
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

    UWorld* World = GetWorld();
    if (!World) return;

    if (bSkillSlotsInitialized)
    {
        TickSkillSlots();
    }

    TickSmoothedGauges(DeltaTime);
    TickHitMarkerSpread(DeltaTime);
    TickCrosshairSpread(DeltaTime);
}

bool ULastFPSHUDWidget::TryInitSkillSlots()
{
    if (bSkillSlotsInitialized)
    {
        return true;
    }

    if (!WBP_SkillCooldownSlot_Q || !WBP_SkillCooldownSlot_E
        || !WBP_SkillCooldownSlot_Z || !WBP_SkillCooldownSlot_F || !CachedASC.IsValid())
    {
        return false;
    }

    const APlayerController* PC = GetOwningPlayer();
    const ALastFPSCharacterBase* Character = PC ? Cast<ALastFPSCharacterBase>(PC->GetPawn()) : nullptr;
    const ULastFPSCharacterDefinition* Definition = Character ? Character->GetCharacterDefinition() : nullptr;
    const UGameInstance* GameInstance = Character ? Character->GetGameInstance() : nullptr;
    const ULastFPSSkillDataSubsystem* SkillDataSubsystem =
        GameInstance ? GameInstance->GetSubsystem<ULastFPSSkillDataSubsystem>() : nullptr;
    if (!Definition || !SkillDataSubsystem)
    {
        return false;
    }

    const FLastFPSCharacterSkillData* Skill1 = SkillDataSubsystem->FindSkill(
        Definition->CharacterId, ELastFPSCharacterSkillSlot::Skill1);
    const FLastFPSCharacterSkillData* Skill2 = SkillDataSubsystem->FindSkill(
        Definition->CharacterId, ELastFPSCharacterSkillSlot::Skill2);
    const FLastFPSCharacterSkillData* Skill3 = SkillDataSubsystem->FindSkill(
        Definition->CharacterId, ELastFPSCharacterSkillSlot::Skill3);
    const FLastFPSCharacterSkillData* Ultimate = SkillDataSubsystem->FindSkill(
        Definition->CharacterId, ELastFPSCharacterSkillSlot::Ultimate);
    const bool bHasDefinitionLoadout = Skill1 && Skill2 && Skill3 && Ultimate;
    if (!bHasDefinitionLoadout)
    {
        WBP_SkillCooldownSlot_Q->ConfigureCooldownSlot(LastFPSGameplayTags::Cooldown_Skill1, nullptr);
        WBP_SkillCooldownSlot_Q->SetKeyLabel(FText::FromString(TEXT("Q")));
        WBP_SkillCooldownSlot_E->ConfigureCooldownSlot(LastFPSGameplayTags::Cooldown_Skill2, nullptr);
        WBP_SkillCooldownSlot_E->SetKeyLabel(FText::FromString(TEXT("E")));
        WBP_SkillCooldownSlot_Z->ConfigureCooldownSlot(LastFPSGameplayTags::Cooldown_Skill3, nullptr);
        WBP_SkillCooldownSlot_Z->SetKeyLabel(FText::FromString(TEXT("Z")));
        WBP_SkillCooldownSlot_F->ConfigureCooldownSlot(LastFPSGameplayTags::Cooldown_Ultimate, nullptr);
        WBP_SkillCooldownSlot_F->SetKeyLabel(FText::FromString(TEXT("F")));
        TickSkillSlots();
        bSkillSlotsInitialized = true;
        return true;
    }

    const auto ConfigureSlot = [](
        ULastFPSSkillCooldownSlotWidget* Widget,
        const FLastFPSCharacterSkillData* SkillData,
        const ELastFPSCharacterSkillSlot ExpectedSlot)
    {
        if (!Widget || !SkillData || SkillData->Slot != ExpectedSlot
            || !SkillData->CooldownTag.IsValid())
        {
            return false;
        }

        Widget->ConfigureCooldownSlot(SkillData->CooldownTag, nullptr);
        Widget->SetConfiguredKeyLabel(SkillData->KeyLabel);
        if (UTexture2D* Icon = SkillData->Icon.LoadSynchronous())
        {
            Widget->SetSkillIconTexture(Icon);
        }
        return true;
    };

    if (!ConfigureSlot(WBP_SkillCooldownSlot_Q, Skill1, ELastFPSCharacterSkillSlot::Skill1)
        || !ConfigureSlot(WBP_SkillCooldownSlot_E, Skill2, ELastFPSCharacterSkillSlot::Skill2)
        || !ConfigureSlot(WBP_SkillCooldownSlot_Z, Skill3, ELastFPSCharacterSkillSlot::Skill3)
        || !ConfigureSlot(WBP_SkillCooldownSlot_F, Ultimate, ELastFPSCharacterSkillSlot::Ultimate))
    {
        return false;
    }

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
    if (WBP_SkillCooldownSlot_Z) { WBP_SkillCooldownSlot_Z->UpdateFromASC(ASC); }
    if (WBP_SkillCooldownSlot_F) { WBP_SkillCooldownSlot_F->UpdateFromASC(ASC); }
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

    Weapon->OnWeaponEquippedChanged.AddUniqueDynamic(this, &ULastFPSHUDWidget::HandleWeaponEquippedChanged);
    OnCrosshairVisibilityChanged(Weapon->HasWeapon());

    BoundWeaponComponent = Weapon;
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

    if (!bAttributeDelegatesBound)
    {
        ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
            .AddUObject(this, &ULastFPSHUDWidget::HandleHealthChanged);
        ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetStaminaAttribute())
            .AddUObject(this, &ULastFPSHUDWidget::HandleStaminaChanged);
        bAttributeDelegatesBound = true;
    }

    HealthGauge.Initialize(AS->GetHealth(), AS->GetMaxHealth());
    StaminaGauge.Initialize(AS->GetStamina(), AS->GetMaxStamina());
    BroadcastHealthDisplay();
    BroadcastStaminaDisplay();

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

void ULastFPSHUDWidget::HandleDamageDealt(
    float DamageAmount,
    float TotalDamageDealt,
    FVector DamageWorldLocation,
    AActor* DamageTargetActor,
    bool bCriticalHit)
{
    SpawnDamageNumber(DamageAmount, TotalDamageDealt, DamageWorldLocation, DamageTargetActor, bCriticalHit);
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

}

void ULastFPSHUDWidget::InitializeHitMarkerMaterial()
{
    if (!HitMarkerImage || HitMarkerMaterial.IsValid())
    {
        return;
    }

    HitMarkerMaterial = HitMarkerImage->GetDynamicMaterial();
}

void ULastFPSHUDWidget::TickHitMarkerSpread(float DeltaTime)
{
    if (!bHitMarkerSpreadAnimating)
    {
        return;
    }

    HitMarkerSpreadElapsed += DeltaTime;

    const float Alpha = FMath::Clamp(
        HitMarkerSpreadElapsed / FMath::Max(HitMarkerSpreadExpandDuration, KINDA_SMALL_NUMBER),
        0.f,
        1.f);

    SetHitMarkerSpread(FMath::Lerp(0.f, HitMarkerMaxSpread, Alpha));

    if (Alpha >= 1.f)
    {
        HideHitMarker();
    }
}

void ULastFPSHUDWidget::SetHitMarkerSpread(float Spread)
{
    InitializeHitMarkerMaterial();

    if (UMaterialInstanceDynamic* Material = HitMarkerMaterial.Get())
    {
        Material->SetScalarParameterValue(HitMarkerSpreadParameterName, Spread);
    }
}

void ULastFPSHUDWidget::InitializeCrosshairMaterial()
{
    if (!CrosshairImage || CrosshairMaterial.IsValid())
    {
        return;
    }

    CrosshairMaterial = CrosshairImage->GetDynamicMaterial();
}

void ULastFPSHUDWidget::TickCrosshairSpread(float DeltaTime)
{
    ALastFPSHero* Hero = nullptr;
    if (const APlayerController* PC = GetOwningPlayer())
    {
        Hero = Cast<ALastFPSHero>(PC->GetPawn());
    }

    float TargetSpread = CrosshairBaseSpread;

    if (Hero)
    {
        const FVector Velocity = Hero->GetVelocity();
        const float HorizontalSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
        if (HorizontalSpeed > CrosshairMovementSpeedThreshold)
        {
            TargetSpread += CrosshairMoveSpread;
        }

        if (const UCharacterMovementComponent* Movement = Hero->GetCharacterMovement())
        {
            if (Movement->IsFalling())
            {
                TargetSpread += CrosshairJumpSpread;
            }
        }

        if (Hero->GetIsADS())
        {
            TargetSpread *= CrosshairZoomSpreadMultiplier;
        }
    }

    FireCrosshairSpread = FMath::FInterpTo(
        FireCrosshairSpread,
        0.f,
        DeltaTime,
        CrosshairFireRecoverSpeed);

    TargetSpread += FireCrosshairSpread;

    CurrentCrosshairSpread = FMath::FInterpTo(
        CurrentCrosshairSpread,
        TargetSpread,
        DeltaTime,
        CrosshairRecoverSpeed);

    SetCrosshairSpread(CurrentCrosshairSpread);
}

void ULastFPSHUDWidget::SetCrosshairSpread(float Spread)
{
    InitializeCrosshairMaterial();

    if (UMaterialInstanceDynamic* Material = CrosshairMaterial.Get())
    {
        Material->SetScalarParameterValue(CrosshairSpreadParameterName, Spread);
    }

    OnCrosshairSpreadChanged(Spread);
}

void ULastFPSHUDWidget::AddCrosshairFireSpread(float SpreadAmount)
{
    const float Amount = SpreadAmount >= 0.f ? SpreadAmount : CrosshairFireSpread;
    FireCrosshairSpread = FMath::Clamp(
        FireCrosshairSpread + Amount,
        0.f,
        CrosshairMaxFireSpread);
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

void ULastFPSHUDWidget::SpawnDamageNumber(
    float DamageAmount,
    float TotalDamageDealt,
    const FVector& DamageWorldLocation,
    AActor* DamageTargetActor,
    bool bCriticalHit)
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC || !DamageNumberWidgetClass || DamageAmount <= 0.f)
    {
        return;
    }

    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
    if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
    {
        return;
    }

    const FVector2D RandomOffset = MakeDamageNumberRandomOffset();

    ULastFPSDamageNumberWidget* DamageNumberWidget =
        CreateWidget<ULastFPSDamageNumberWidget>(PC, DamageNumberWidgetClass);
    if (!DamageNumberWidget)
    {
        return;
    }

    DamageNumberWidget->AddToViewport(20);
    DamageNumberWidget->InitializeDamageNumber(
        DamageAmount,
        TotalDamageDealt,
        DamageTargetActor,
        DamageWorldLocation,
        DamageNumberWorldOffset,
        DamageNumberScreenOffset,
        RandomOffset,
        bCriticalHit);
}

FVector2D ULastFPSHUDWidget::MakeDamageNumberRandomOffset() const
{
    const float RandomAngle = FMath::FRandRange(0.f, 2.f * PI);
    const float RandomDistance = DamageNumberRandomRadiusOffset + FMath::FRandRange(0.f, DamageNumberRandomRadius);

    return FVector2D(
        FMath::Cos(RandomAngle) * RandomDistance,
        FMath::Sin(RandomAngle) * RandomDistance);
}

void ULastFPSHUDWidget::HandleWeaponEquippedChanged(bool bEquipped)
{
    OnCrosshairVisibilityChanged(bEquipped);
}

void ULastFPSHUDWidget::ShowHitMarker()
{
    if (!HitMarkerImage)
        return;

    HitMarkerSpreadElapsed = 0.f;
    bHitMarkerSpreadAnimating = true;
    SetHitMarkerSpread(0.f);

    HitMarkerImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULastFPSHUDWidget::HideHitMarker()
{
    if (HitMarkerImage)
        HitMarkerImage->SetVisibility(ESlateVisibility::Collapsed);

    bHitMarkerSpreadAnimating = false;
    HitMarkerSpreadElapsed = 0.f;
    SetHitMarkerSpread(0.f);
}
