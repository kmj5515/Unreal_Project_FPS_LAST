#include "UI/HUD/Presenters/LastFPSAmmoPresenter.h"

#include "Character/Components/WeaponComponent.h"
#include "Components/TextBlock.h"
#include "Localization/LastFPSLocalization.h"

void ULastFPSAmmoPresenter::Initialize(
    UTextBlock* InCurrentAmmoText,
    UTextBlock* InReserveAmmoText,
    UTextBlock* InCombinedAmmoText,
    UWidget* InAmmoInfoLayer)
{
    CurrentAmmoText  = InCurrentAmmoText;
    ReserveAmmoText  = InReserveAmmoText;
    CombinedAmmoText = InCombinedAmmoText;
    AmmoInfoLayer    = InAmmoInfoLayer;
}

void ULastFPSAmmoPresenter::BindToWeaponComponent(UWeaponComponent* InWeaponComponent)
{
    if (WeaponComponent.Get() == InWeaponComponent)
    {
        return;
    }

    Reset();

    if (!InWeaponComponent)
    {
        return;
    }

    WeaponComponent = InWeaponComponent;
    InWeaponComponent->OnWeaponAmmoChanged.AddUniqueDynamic(this, &ULastFPSAmmoPresenter::HandleAmmoChanged);
    InWeaponComponent->OnWeaponReserveAmmoChanged.AddUniqueDynamic(this, &ULastFPSAmmoPresenter::HandleReserveAmmoChanged);
    InWeaponComponent->OnWeaponEquippedChanged.AddUniqueDynamic(this, &ULastFPSAmmoPresenter::HandleWeaponEquippedChanged);

    CachedCurrentAmmo      = InWeaponComponent->GetCurrentMagazineAmmo();
    CachedMagazineCapacity = InWeaponComponent->GetMagazineCapacity();
    CachedReserveAmmo      = InWeaponComponent->GetCurrentReserveAmmo();
    bHasWeapon             = true;

    UpdateDisplay();
}

void ULastFPSAmmoPresenter::Reset()
{
    if (UWeaponComponent* WeaponComp = WeaponComponent.Get())
    {
        WeaponComp->OnWeaponAmmoChanged.RemoveDynamic(this, &ULastFPSAmmoPresenter::HandleAmmoChanged);
        WeaponComp->OnWeaponReserveAmmoChanged.RemoveDynamic(this, &ULastFPSAmmoPresenter::HandleReserveAmmoChanged);
        WeaponComp->OnWeaponEquippedChanged.RemoveDynamic(this, &ULastFPSAmmoPresenter::HandleWeaponEquippedChanged);
    }

    WeaponComponent = nullptr;
    bHasWeapon      = false;
    CachedCurrentAmmo      = 0;
    CachedMagazineCapacity = 0;
    CachedReserveAmmo      = 0;

    UpdateDisplay();
}

void ULastFPSAmmoPresenter::HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineCapacity)
{
    CachedCurrentAmmo      = CurrentAmmo;
    CachedMagazineCapacity = MagazineCapacity;
    UpdateDisplay();
}

void ULastFPSAmmoPresenter::HandleReserveAmmoChanged(int32 ReserveAmmo)
{
    CachedReserveAmmo = ReserveAmmo;
    UpdateDisplay();
}

void ULastFPSAmmoPresenter::HandleWeaponEquippedChanged(bool bEquipped)
{
    bHasWeapon = bEquipped;
    if (bEquipped && WeaponComponent.IsValid())
    {
        CachedCurrentAmmo      = WeaponComponent->GetCurrentMagazineAmmo();
        CachedMagazineCapacity = WeaponComponent->GetMagazineCapacity();
        CachedReserveAmmo      = WeaponComponent->GetCurrentReserveAmmo();
    }
    UpdateDisplay();
}

void ULastFPSAmmoPresenter::UpdateDisplay()
{
    const FText CurrentText = bHasWeapon
       ? FText::AsNumber(CachedCurrentAmmo)
       : FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::NotAvailable);

    const FText ReserveText = bHasWeapon
        ? FText::AsNumber(CachedReserveAmmo)
        : FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::NotAvailable);

    const FText CombinedText = bHasWeapon
        ? FText::Format(
            FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::RatioFormat),
            FText::AsNumber(CachedCurrentAmmo),
            FText::AsNumber(CachedReserveAmmo))
        : FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::NotAvailable);

    
    if (UTextBlock* CurrentWidget = CurrentAmmoText.Get())
    {
        CurrentWidget->SetText(CurrentText);
    }

    if (UTextBlock* ReserveWidget = ReserveAmmoText.Get())
    {
        ReserveWidget->SetText(ReserveText);
    }

    if (UTextBlock* CombinedWidget = CombinedAmmoText.Get())
    {
        CombinedWidget->SetText(CombinedText);
    }
    
    if (UWidget* Layer = AmmoInfoLayer.Get())
    {
        Layer->SetVisibility(bHasWeapon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}
