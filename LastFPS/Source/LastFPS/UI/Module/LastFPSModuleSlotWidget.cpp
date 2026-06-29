#include "UI/Module/LastFPSModuleSlotWidget.h"

#include "UI/Inventory/LastFPSItemSlotWidget.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void ULastFPSModuleSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Slot)
	{
		Button_Slot->OnClicked().AddUObject(this, &ULastFPSModuleSlotWidget::HandleSlotClicked);
	}
}

void ULastFPSModuleSlotWidget::SetupEquipped(int32 InSlotIndex, const FLastFPSItemData& InItem, const FText& StatText)
{
	SlotIndex = InSlotIndex;

	if (Img_Empty)
	{
		Img_Empty->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Image_Icon)
	{
		if (UTexture2D* Tex = InItem.Icon.LoadSynchronous())
		{
			Image_Icon->SetBrushFromTexture(Tex);
		}
		Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (Img_RarityBorder)
	{
		Img_RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		Img_RarityBorder->SetColorAndOpacity(ULastFPSItemSlotWidget::RarityToColor(InItem.Rarity));
	}

	if (TB_ModuleName)
	{
		TB_ModuleName->SetVisibility(ESlateVisibility::HitTestInvisible);
		TB_ModuleName->SetText(InItem.ItemName);
	}

	if (TB_Stats)
	{
		TB_Stats->SetVisibility(ESlateVisibility::HitTestInvisible);
		TB_Stats->SetText(StatText);
	}

	if (Button_Slot)
	{
		Button_Slot->SetIsEnabled(true);
	}
}

void ULastFPSModuleSlotWidget::SetEmpty(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	if (Img_Empty)
	{
		Img_Empty->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Image_Icon)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Img_RarityBorder)
	{
		Img_RarityBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (TB_ModuleName)
	{
		TB_ModuleName->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (TB_Stats)
	{
		TB_Stats->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Button_Slot)
	{
		// 빈 슬롯은 해제할 게 없으므로 비활성 (장착은 좌측 목록에서)
		Button_Slot->SetIsEnabled(false);
	}
}

void ULastFPSModuleSlotWidget::HandleSlotClicked()
{
	if (SlotIndex != INDEX_NONE)
	{
		OnSlotClicked.ExecuteIfBound(SlotIndex);
	}
}
