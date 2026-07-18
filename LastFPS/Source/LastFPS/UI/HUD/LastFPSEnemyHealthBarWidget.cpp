#include "UI/HUD/LastFPSEnemyHealthBarWidget.h"

#include "AbilitySystemComponent.h"
#include "LastFPSStatusEffectListWidget.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/LastFPSCharacterBase.h"
#include "Components/ProgressBar.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "GameFramework/PlayerController.h"

ULastFPSEnemyHealthBarWidget::ULastFPSEnemyHealthBarWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetVisibility(ESlateVisibility::Collapsed);
}

void ULastFPSEnemyHealthBarWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureNativeWidgets();

    if (PB_EnemyHealth)
    {
        // 앞쪽 현재 HP Bar의 빈 영역이 뒤쪽 피해 잔상 Bar를 가리지 않게 한다.
        FProgressBarStyle CurrentHealthStyle = PB_EnemyHealth->GetWidgetStyle();
        CurrentHealthStyle.BackgroundImage.TintColor = FSlateColor(FLinearColor::Transparent);
        PB_EnemyHealth->SetWidgetStyle(CurrentHealthStyle);
    }
}

void ULastFPSEnemyHealthBarWidget::NativeDestruct()
{
    UnbindFromEnemy();
    Super::NativeDestruct();
}

void ULastFPSEnemyHealthBarWidget::InitializeForEnemy(
    ALastFPSCharacterBase* Enemy,
    const FLastFPSEnemyHealthBarSettings& Settings,
    const float InitialDamageAmount)
{
    if (!IsValid(Enemy) || Enemy->IsPlayerControlled() || !Enemy->IsAlive())
    {
        ReleaseFromEnemy();
        return;
    }

    if (TrackedEnemy.Get() != Enemy)
    {
        UnbindFromEnemy();
        BindToEnemy(Enemy);
    }

    if (PB_EnemyHealthDamageTrail)
    {
        PB_EnemyHealthDamageTrail->SetFillColorAndOpacity(Settings.DamageTrailFillColor);
    }
    
    if (WBP_StatusEffectList)
    {
        WBP_StatusEffectList->InitializeWithAbilitySystem(BoundASC.Get());
    }
    
    RefreshDisplayDuration(Settings.DisplayDuration);
    RefreshHealthDisplay();
    DamageTrailHealth = CurrentHealth;
    NotifyDamage(InitialDamageAmount, Settings);
    UpdateScreenPosition(Settings);
}

void ULastFPSEnemyHealthBarWidget::InitializeForFixedHUDTarget(
    ALastFPSCharacterBase* Enemy,
    const FLastFPSEnemyHealthBarSettings& Settings,
    const float InitialDamageAmount)
{
    if (!IsValid(Enemy) || Enemy->IsPlayerControlled() || !Enemy->IsAlive())
    {
        ReleaseFromEnemy();
        return;
    }

    if (TrackedEnemy.Get() != Enemy)
    {
        UnbindFromEnemy();
        BindToEnemy(Enemy);
    }

    if (PB_EnemyHealthDamageTrail)
    {
        PB_EnemyHealthDamageTrail->SetFillColorAndOpacity(Settings.DamageTrailFillColor);
    }

    if (WBP_StatusEffectList)
    {
        WBP_StatusEffectList->InitializeWithAbilitySystem(BoundASC.Get());
    }

    RefreshHealthDisplay();
    DamageTrailHealth = CurrentHealth;
    NotifyDamage(InitialDamageAmount, Settings);
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULastFPSEnemyHealthBarWidget::NotifyDamage(
    const float DamageAmount,
    const FLastFPSEnemyHealthBarSettings& Settings)
{
    ALastFPSCharacterBase* Enemy = TrackedEnemy.Get();
    if (!Enemy || DamageAmount <= 0.f)
    {
        return;
    }

    CurrentHealth = FMath::Clamp(Enemy->GetHealth(), 0.f, FMath::Max(Enemy->GetMaxHealth(), 0.f));
    CurrentMaxHealth = FMath::Max(Enemy->GetMaxHealth(), KINDA_SMALL_NUMBER);

    // 첫 피해로 위젯이 생성된 경우에도 피격 직전 체력을 복원해 손실 구간을 표시한다.
    DamageTrailHealth = FMath::Clamp(
        FMath::Max(DamageTrailHealth, CurrentHealth + DamageAmount),
        CurrentHealth,
        CurrentMaxHealth);
    DamageTrailHoldRemaining = FMath::Max(Settings.DamageTrailHoldDuration, 0.f);
    ApplyHealthBars();
}

void ULastFPSEnemyHealthBarWidget::RefreshDisplayDuration(const float DisplayDuration)
{
    RemainingDisplayTime = FMath::Max(DisplayDuration, 0.f);
}

void ULastFPSEnemyHealthBarWidget::ReleaseFromEnemy()
{
    UnbindFromEnemy();
    RemainingDisplayTime = 0.f;
    CurrentHealth = 0.f;
    CurrentMaxHealth = 1.f;
    DamageTrailHealth = 0.f;
    DamageTrailHoldRemaining = 0.f;
    
    if (WBP_StatusEffectList)
    {
        WBP_StatusEffectList->UninitializeFromAbilitySystem();
    }
    
    SetVisibility(ESlateVisibility::Collapsed);
}

bool ULastFPSEnemyHealthBarWidget::UpdateTrackedEnemy(
    const float DeltaTime,
    const FLastFPSEnemyHealthBarSettings& Settings)
{
    ALastFPSCharacterBase* Enemy = TrackedEnemy.Get();
    if (!IsValid(Enemy) || !Enemy->IsAlive())
    {
        ReleaseFromEnemy();
        return false;
    }

    RemainingDisplayTime = FMath::Max(RemainingDisplayTime - FMath::Max(DeltaTime, 0.f), 0.f);
    if (RemainingDisplayTime <= 0.f)
    {
        ReleaseFromEnemy();
        return false;
    }

    UpdateDamageTrail(DeltaTime, Settings);
    if (WBP_StatusEffectList)
    {
        WBP_StatusEffectList->UpdateRuntimeStates();
    }
    UpdateScreenPosition(Settings);
    return true;
}

bool ULastFPSEnemyHealthBarWidget::UpdateFixedHUDTarget(
    const float DeltaTime,
    const FLastFPSEnemyHealthBarSettings& Settings)
{
    ALastFPSCharacterBase* Enemy = TrackedEnemy.Get();
    if (!IsValid(Enemy) || !Enemy->IsAlive())
    {
        ReleaseFromEnemy();
        return false;
    }

    UpdateDamageTrail(DeltaTime, Settings);
    if (WBP_StatusEffectList)
    {
        WBP_StatusEffectList->UpdateRuntimeStates();
    }
    return true;
}

bool ULastFPSEnemyHealthBarWidget::IsTrackingEnemy(const ALastFPSCharacterBase* Enemy) const
{
    return Enemy && TrackedEnemy.Get() == Enemy;
}

void ULastFPSEnemyHealthBarWidget::EnsureNativeWidgets()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EnemyHealthRoot"));
    TXT_EnemyName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_EnemyName"));
    UOverlay* HealthOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("EnemyHealthOverlay"));
    PB_EnemyHealthDamageTrail = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("PB_EnemyHealthDamageTrail"));
    PB_EnemyHealth = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PB_EnemyHealth"));
    WidgetTree->RootWidget = RootBox;

    TXT_EnemyName->SetJustification(ETextJustify::Center);
    PB_EnemyHealthDamageTrail->SetFillColorAndOpacity(FLinearColor(1.f, 0.78f, 0.18f, 1.f));
    PB_EnemyHealth->SetFillColorAndOpacity(FLinearColor(0.906f, 0.298f, 0.235f, 1.f));
    RootBox->AddChildToVerticalBox(TXT_EnemyName);
    RootBox->AddChildToVerticalBox(HealthOverlay);
    HealthOverlay->AddChildToOverlay(PB_EnemyHealthDamageTrail);
    HealthOverlay->AddChildToOverlay(PB_EnemyHealth);
}

void ULastFPSEnemyHealthBarWidget::BindToEnemy(ALastFPSCharacterBase* Enemy)
{
    TrackedEnemy = Enemy;
    UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
    if (!ASC)
    {
        return;
    }

    BoundASC = ASC;
    HealthDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
        ULastFPSAttributeSet::GetHealthAttribute()).AddUObject(
            this, &ULastFPSEnemyHealthBarWidget::HandleHealthChanged);
    MaxHealthDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
        ULastFPSAttributeSet::GetMaxHealthAttribute()).AddUObject(
            this, &ULastFPSEnemyHealthBarWidget::HandleMaxHealthChanged);
}

void ULastFPSEnemyHealthBarWidget::UnbindFromEnemy()
{
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        if (HealthDelegateHandle.IsValid())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(
                ULastFPSAttributeSet::GetHealthAttribute()).Remove(HealthDelegateHandle);
        }
        if (MaxHealthDelegateHandle.IsValid())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(
                ULastFPSAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthDelegateHandle);
        }
    }

    HealthDelegateHandle.Reset();
    MaxHealthDelegateHandle.Reset();
    BoundASC.Reset();
    TrackedEnemy.Reset();
}

void ULastFPSEnemyHealthBarWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
    if (Data.NewValue <= 0.f)
    {
        // 마지막 타격 알림보다 먼저 사망이 복제되어도 빈 Bar가 다시 살아나지 않게 즉시 풀로 반환한다.
        ReleaseFromEnemy();
        return;
    }

    RefreshHealthDisplay();
}

void ULastFPSEnemyHealthBarWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& /*Data*/)
{
    RefreshHealthDisplay();
}

void ULastFPSEnemyHealthBarWidget::RefreshHealthDisplay()
{
    const ALastFPSCharacterBase* Enemy = TrackedEnemy.Get();
    if (!Enemy)
    {
        return;
    }

    const float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Max(Enemy->GetHealth(), 0.f);
    CurrentMaxHealth = FMath::Max(Enemy->GetMaxHealth(), KINDA_SMALL_NUMBER);
    if (CurrentHealth > PreviousHealth)
    {
        // 회복은 피해 잔상으로 보이지 않도록 두 게이지를 함께 올린다.
        DamageTrailHealth = CurrentHealth;
        DamageTrailHoldRemaining = 0.f;
    }
    else
    {
        DamageTrailHealth = FMath::Clamp(
            FMath::Max(DamageTrailHealth, CurrentHealth),
            CurrentHealth,
            CurrentMaxHealth);
    }
    const FText DisplayName = ResolveDisplayName();

    EnsureNativeWidgets();
    ApplyHealthBars();
    if (TXT_EnemyName)
    {
        TXT_EnemyName->SetText(DisplayName);
    }
    OnEnemyHealthChanged(CurrentHealth, CurrentMaxHealth, DisplayName);
}

void ULastFPSEnemyHealthBarWidget::UpdateDamageTrail(
    const float DeltaTime,
    const FLastFPSEnemyHealthBarSettings& Settings)
{
    const float SafeDeltaTime = FMath::Max(DeltaTime, 0.f);
    if (DamageTrailHoldRemaining > 0.f)
    {
        DamageTrailHoldRemaining = FMath::Max(DamageTrailHoldRemaining - SafeDeltaTime, 0.f);
        return;
    }

    if (DamageTrailHealth <= CurrentHealth || FMath::IsNearlyEqual(DamageTrailHealth, CurrentHealth))
    {
        DamageTrailHealth = CurrentHealth;
        return;
    }

    const float DrainDuration = FMath::Max(Settings.DamageTrailDrainDuration, KINDA_SMALL_NUMBER);
    const float DrainSpeed = CurrentMaxHealth / DrainDuration;
    DamageTrailHealth = FMath::FInterpConstantTo(
        DamageTrailHealth, CurrentHealth, SafeDeltaTime, DrainSpeed);
    ApplyHealthBars();
}

void ULastFPSEnemyHealthBarWidget::ApplyHealthBars()
{
    const float SafeMaxHealth = FMath::Max(CurrentMaxHealth, KINDA_SMALL_NUMBER);
    if (PB_EnemyHealthDamageTrail)
    {
        PB_EnemyHealthDamageTrail->SetPercent(
            FMath::Clamp(DamageTrailHealth / SafeMaxHealth, 0.f, 1.f));
    }
    if (PB_EnemyHealth)
    {
        PB_EnemyHealth->SetPercent(FMath::Clamp(CurrentHealth / SafeMaxHealth, 0.f, 1.f));
    }
}

bool ULastFPSEnemyHealthBarWidget::UpdateScreenPosition(
    const FLastFPSEnemyHealthBarSettings& Settings)
{
    ALastFPSCharacterBase* Enemy = TrackedEnemy.Get();
    APlayerController* PC = GetOwningPlayer();
    if (!Enemy || !PC)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return false;
    }

    const FVector DisplayLocation = Enemy->GetActorLocation() + Settings.WorldOffset;
    if (Settings.MaxDisplayDistance > 0.f
        && FVector::DistSquared(PC->GetFocalLocation(), DisplayLocation)
            > FMath::Square(Settings.MaxDisplayDistance))
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return false;
    }

    if (PC->PlayerCameraManager)
    {
        const FVector CameraToTarget = DisplayLocation - PC->PlayerCameraManager->GetCameraLocation();
        if (FVector::DotProduct(CameraToTarget, PC->PlayerCameraManager->GetCameraRotation().Vector()) <= 0.f)
        {
            SetVisibility(ESlateVisibility::Collapsed);
            return false;
        }
    }

    FVector2D ScreenPosition;
    if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, DisplayLocation, ScreenPosition, true))
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return false;
    }

    const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this)
        / FMath::Max(ViewportScale, KINDA_SMALL_NUMBER);
    ScreenPosition += Settings.ScreenOffset;
    const float ViewportPadding = FMath::Max(Settings.ScreenVisibilityPadding, 0.f);
    if (ScreenPosition.X < -ViewportPadding || ScreenPosition.Y < -ViewportPadding
        || ScreenPosition.X > ViewportSize.X + ViewportPadding
        || ScreenPosition.Y > ViewportSize.Y + ViewportPadding)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return false;
    }

    SetAlignmentInViewport(FVector2D(0.5f, 1.f));
    SetPositionInViewport(ScreenPosition, false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    return true;
}

FText ULastFPSEnemyHealthBarWidget::ResolveDisplayName() const
{
    const ALastFPSCharacterBase* Enemy = TrackedEnemy.Get();
    if (!Enemy)
    {
        return FText::GetEmpty();
    }

    if (const ULastFPSCharacterDefinition* Definition = Enemy->GetCharacterDefinition();
        Definition && !Definition->DisplayName.IsEmpty())
    {
        return Definition->DisplayName;
    }

    return FText::FromString(Enemy->GetKillFeedDisplayName());
}
