#include "UI/LastFPSHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Game/LastFPSMatchGameState.h"
#include "Game/LastFPSPlayerState.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "TimerManager.h"

void ULastFPSHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (HitMarkerImage)
        HitMarkerImage->SetVisibility(ESlateVisibility::Collapsed);

    if (!InitializeHUD())
    {
        // 클라이언트에서 PlayerState 복제가 아직 안 된 경우 — 0.1초마다 재시도
        GetWorld()->GetTimerManager().SetTimer(
            RetryTimerHandle, this, &ULastFPSHUDWidget::RetryInitialize, 0.1f, true);
    }

    TryBindKillFeed();
}

void ULastFPSHUDWidget::NativeDestruct()
{
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
    if (InitializeHUD())
        GetWorld()->GetTimerManager().ClearTimer(RetryTimerHandle);

    TryBindKillFeed();
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

void ULastFPSHUDWidget::AddKillFeedLine(const FString& KillerName, const FString& VictimName)
{
    if (!KillFeedContainer)
        return;

    UTextBlock* Line = NewObject<UTextBlock>(this);
    if (!Line)
        return;

    Line->SetText(FText::FromString(FString::Printf(TEXT("%s  →  %s"), *KillerName, *VictimName)));

    if (UVerticalBox* VBox = Cast<UVerticalBox>(KillFeedContainer))
    {
        VBox->AddChildToVerticalBox(Line);
    }
    else
    {
        KillFeedContainer->AddChild(Line);
    }

    KillFeedLines.Add(Line);

    while (KillFeedLines.Num() > MaxKillFeedEntries)
    {
        if (UTextBlock* Oldest = KillFeedLines[0])
        {
            Oldest->RemoveFromParent();
        }
        KillFeedLines.RemoveAt(0);
    }
}

void ULastFPSHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!MatchTimerText) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const ALastFPSMatchGameState* MGS = World->GetGameState<ALastFPSMatchGameState>();
    if (!MGS) return;

    const int32 TotalSec = FMath::Max(0, FMath::CeilToInt(MGS->GetMatchTimeRemaining()));
    if (TotalSec == CachedMatchTimeIntSec) return;

    CachedMatchTimeIntSec = TotalSec;

    const int32 Min = TotalSec / 60;
    const int32 Sec = TotalSec % 60;
    MatchTimerText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Min, Sec)));
}

bool ULastFPSHUDWidget::InitializeHUD()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return false;

    ALastFPSPlayerState* PS = PC->GetPlayerState<ALastFPSPlayerState>();
    if (!PS) return false;

    UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
    const ULastFPSAttributeSet* AS = PS->GetAttributeSet();
    if (!ASC || !AS) return false;

    // ── GAS 어트리뷰트 델리게이트 바인딩 ────────────────────────
    ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
        .AddUObject(this, &ULastFPSHUDWidget::HandleHealthChanged);

    ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetStaminaAttribute())
        .AddUObject(this, &ULastFPSHUDWidget::HandleStaminaChanged);

    ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetUltimateGaugeAttribute())
        .AddUObject(this, &ULastFPSHUDWidget::HandleUltimateGaugeChanged);

    CachedMaxHealth        = AS->GetMaxHealth();
    CachedMaxStamina       = AS->GetMaxStamina();
    CachedMaxUltimateGauge = AS->GetMaxUltimateGauge();

    // 바인딩 직후 현재값으로 바 초기화
    OnHealthChanged(AS->GetHealth(), CachedMaxHealth);
    OnStaminaChanged(AS->GetStamina(), CachedMaxStamina);
    OnUltimateGaugeChanged(AS->GetUltimateGauge(), CachedMaxUltimateGauge);

    // ── WeaponComponent 오버히트 / 크로스헤어 델리게이트 바인딩 ─────
    // Pawn도 아직 빙의 안 됐을 수 있으므로 실패해도 ASC 바인딩은 유지
    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(PC->GetPawn()))
    {
        if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
        {
            Weapon->OnHeatChanged.AddDynamic(this, &ULastFPSHUDWidget::HandleHeatChanged);
            OnHeatChanged(Weapon->GetCurrentHeat(), Weapon->GetMaxHeat(), Weapon->IsOverheated());

            Weapon->OnWeaponEquippedChanged.AddDynamic(this, &ULastFPSHUDWidget::HandleWeaponEquippedChanged);
            OnCrosshairVisibilityChanged(Weapon->HasWeapon());
        }
    }

    return true;
}

void ULastFPSHUDWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
    OnHealthChanged(Data.NewValue, CachedMaxHealth);
}

void ULastFPSHUDWidget::HandleStaminaChanged(const FOnAttributeChangeData& Data)
{
    OnStaminaChanged(Data.NewValue, CachedMaxStamina);
}

void ULastFPSHUDWidget::HandleUltimateGaugeChanged(const FOnAttributeChangeData& Data)
{
    OnUltimateGaugeChanged(Data.NewValue, CachedMaxUltimateGauge);
}

void ULastFPSHUDWidget::HandleHeatChanged(float Current, float Max, bool bIsOverheated)
{
    OnHeatChanged(Current, Max, bIsOverheated);
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
