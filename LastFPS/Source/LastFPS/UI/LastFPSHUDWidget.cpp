#include "UI/LastFPSHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Game/LastFPSPlayerState.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"

void ULastFPSHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!InitializeHUD())
    {
        // 클라이언트에서 PlayerState 복제가 아직 안 된 경우 — 0.1초마다 재시도
        GetWorld()->GetTimerManager().SetTimer(
            RetryTimerHandle, this, &ULastFPSHUDWidget::RetryInitialize, 0.1f, true);
    }
}

void ULastFPSHUDWidget::RetryInitialize()
{
    if (InitializeHUD())
        GetWorld()->GetTimerManager().ClearTimer(RetryTimerHandle);
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

    // 바인딩 직후 현재값으로 바 초기화
    OnHealthChanged(AS->GetHealth(), AS->GetMaxHealth());
    OnStaminaChanged(AS->GetStamina(), AS->GetMaxStamina());
    OnUltimateGaugeChanged(AS->GetUltimateGauge(), AS->GetMaxUltimateGauge());

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
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    if (const ALastFPSPlayerState* PS = PC->GetPlayerState<ALastFPSPlayerState>())
        OnHealthChanged(Data.NewValue, PS->GetAttributeSet()->GetMaxHealth());
}

void ULastFPSHUDWidget::HandleStaminaChanged(const FOnAttributeChangeData& Data)
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    if (const ALastFPSPlayerState* PS = PC->GetPlayerState<ALastFPSPlayerState>())
        OnStaminaChanged(Data.NewValue, PS->GetAttributeSet()->GetMaxStamina());
}

void ULastFPSHUDWidget::HandleUltimateGaugeChanged(const FOnAttributeChangeData& Data)
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    if (const ALastFPSPlayerState* PS = PC->GetPlayerState<ALastFPSPlayerState>())
        OnUltimateGaugeChanged(Data.NewValue, PS->GetAttributeSet()->GetMaxUltimateGauge());
}

void ULastFPSHUDWidget::HandleHeatChanged(float Current, float Max, bool bIsOverheated)
{
    OnHeatChanged(Current, Max, bIsOverheated);
}

void ULastFPSHUDWidget::HandleWeaponEquippedChanged(bool bEquipped)
{
    OnCrosshairVisibilityChanged(bEquipped);
}
