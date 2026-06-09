#include "UI/LastFPSItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void ULastFPSItemSlotWidget::SetupSlot(const FLastFPSItemData& InItem)
{
	if (Image_Icon)
	{
		// 아이콘은 소프트 레퍼런스 — 동기 로드(프로토). 추후 AsyncLoad로 교체.
		if (UTexture2D* Tex = InItem.Icon.LoadSynchronous())
		{
			Image_Icon->SetBrushFromTexture(Tex);
		}
		Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (TB_ItemName)
	{
		TB_ItemName->SetText(InItem.ItemName);
	}

	if (TB_Rarity)
	{
		TB_Rarity->SetText(UEnum::GetDisplayValueAsText(InItem.Rarity));
	}

	OnSlotDisplayed(InItem.ItemType, InItem.Rarity);
}

void ULastFPSItemSlotWidget::SetEmpty()
{
	if (Image_Icon)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Hidden);
	}
	if (TB_ItemName)
	{
		TB_ItemName->SetText(FText::GetEmpty());
	}
	if (TB_Rarity)
	{
		TB_Rarity->SetText(FText::GetEmpty());
	}

	OnSlotCleared();
}
