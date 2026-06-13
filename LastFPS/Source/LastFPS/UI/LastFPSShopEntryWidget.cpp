#include "UI/LastFPSShopEntryWidget.h"

#include "UI/LastFPSItemSlotWidget.h"
#include "UI/LastFPSButtonBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void ULastFPSShopEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Buy)
	{
		Button_Buy->OnClicked().AddUObject(this, &ULastFPSShopEntryWidget::HandleBuyClicked);
	}
}

void ULastFPSShopEntryWidget::SetupShopItem(const FLastFPSShopItemData& InItem, FName InRowName)
{
	RowName = InRowName;
	Price = InItem.Price;

	if (Image_Icon)
	{
		if (UTexture2D* Tex = InItem.Icon.LoadSynchronous())
		{
			Image_Icon->SetBrushFromTexture(Tex);
			Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Img_RarityBorder)
	{
		Img_RarityBorder->SetColorAndOpacity(ULastFPSItemSlotWidget::RarityToColor(InItem.Rarity));
	}

	if (TB_ItemName)
	{
		TB_ItemName->SetText(InItem.ItemName);
	}

	if (TB_Description)
	{
		TB_Description->SetText(InItem.Description);
	}

	if (TB_Rarity)
	{
		TB_Rarity->SetText(UEnum::GetDisplayValueAsText(InItem.Rarity));
		TB_Rarity->SetColorAndOpacity(FSlateColor(ULastFPSItemSlotWidget::RarityToColor(InItem.Rarity)));
	}

	if (TB_Price)
	{
		TB_Price->SetText(FText::AsNumber(Price));
	}
}

void ULastFPSShopEntryWidget::SetAffordable(bool bAffordable)
{
	if (Button_Buy)
	{
		Button_Buy->SetIsEnabled(bAffordable);
	}
}

void ULastFPSShopEntryWidget::HandleBuyClicked()
{
	OnBuyClicked.ExecuteIfBound(RowName);
}
