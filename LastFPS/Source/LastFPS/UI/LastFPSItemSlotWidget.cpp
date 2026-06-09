#include "UI/LastFPSItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void ULastFPSItemSlotWidget::SetupSlot(const FLastFPSItemData& InItem)
{
	// 배경 숨기고 희귀도 테두리 표시
	if (Img_Background)
	{
		Img_Background->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Img_RarityBorder)
	{
		Img_RarityBorder->SetColorAndOpacity(RarityToColor(InItem.Rarity));
		Img_RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 아이콘 — 소프트 레퍼런스 동기 로드(프로토). 추후 AsyncLoad로 교체.
	if (Image_Icon)
	{
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
		TB_Rarity->SetColorAndOpacity(FSlateColor(RarityToColor(InItem.Rarity)));
	}
}

void ULastFPSItemSlotWidget::SetEmpty()
{
	if (Img_Background)
	{
		Img_Background->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Img_RarityBorder)
	{
		Img_RarityBorder->SetVisibility(ESlateVisibility::Hidden);
	}
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
}

FLinearColor ULastFPSItemSlotWidget::RarityToColor(ELastFPSItemRarity Rarity)
{
	switch (Rarity)
	{
	case ELastFPSItemRarity::Common:    return FLinearColor(0.5f,  0.5f,  0.5f,  1.f); // 회색
	case ELastFPSItemRarity::Rare:      return FLinearColor(0.1f,  0.4f,  1.f,   1.f); // 파랑
	case ELastFPSItemRarity::Epic:      return FLinearColor(0.6f,  0.1f,  1.f,   1.f); // 보라
	case ELastFPSItemRarity::Legendary: return FLinearColor(1.f,   0.5f,  0.05f, 1.f); // 주황
	default:                            return FLinearColor::White;
	}
}
