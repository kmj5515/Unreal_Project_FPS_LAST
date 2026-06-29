#include "UI/Module/LastFPSModuleEntryWidget.h"

#include "UI/Inventory/LastFPSItemSlotWidget.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void ULastFPSModuleEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Equip)
	{
		Button_Equip->OnClicked().AddUObject(this, &ULastFPSModuleEntryWidget::HandleEquipClicked);
	}
}

void ULastFPSModuleEntryWidget::SetupModule(
	const FLastFPSItemData& InItem,
	const FText& StatText,
	int32 CapacityCost,
	int32 Count,
	FName InRowName)
{
	RowName = InRowName;

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

	if (TB_ModuleName)
	{
		TB_ModuleName->SetText(InItem.ItemName);
	}

	if (TB_Stats)
	{
		TB_Stats->SetText(StatText);
	}

	if (TB_Capacity)
	{
		TB_Capacity->SetText(FText::AsNumber(CapacityCost));
	}

	if (TB_Count)
	{
		if (Count > 1)
		{
			TB_Count->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Count)));
			TB_Count->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			TB_Count->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ULastFPSModuleEntryWidget::SetEquipEnabled(bool bEnabled)
{
	if (Button_Equip)
	{
		Button_Equip->SetIsEnabled(bEnabled);
	}
}

void ULastFPSModuleEntryWidget::HandleEquipClicked()
{
	OnEquipClicked.ExecuteIfBound(RowName);
}
