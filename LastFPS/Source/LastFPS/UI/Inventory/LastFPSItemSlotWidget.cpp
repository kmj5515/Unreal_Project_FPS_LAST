#include "UI/Inventory/LastFPSItemSlotWidget.h"

#include "UI/Inventory/LastFPSItemTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void ULastFPSItemSlotWidget::SetupSlot(const FLastFPSItemData& InItem, FName InRowId, int32 Count)
{
	ItemRowId = InRowId;

	// hover(마우스 진입/이탈) 이벤트와 툴팁이 동작하려면 슬롯 루트가 히트테스트 가능해야 한다.
	SetVisibility(ESlateVisibility::Visible);

	// 공용 툴팁 위젯을 만들어 이 슬롯에 붙인다(hover 시 UMG 가 자동 표시/추적).
	if (TooltipWidgetClass)
	{
		if (ULastFPSItemTooltipWidget* Tooltip = CreateWidget<ULastFPSItemTooltipWidget>(this, TooltipWidgetClass))
		{
			Tooltip->SetupTooltip(InItem, InRowId);
			SetToolTip(Tooltip);
		}
	}

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

	if (TB_Count)
	{
		if (Count > 1)
		{
			TB_Count->SetText(FText::Format(NSLOCTEXT("LastFPS", "Inventory_Count", "x{0}"), FText::AsNumber(Count)));
			TB_Count->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			TB_Count->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ULastFPSItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	OnHovered.ExecuteIfBound(ItemRowId);
}

void ULastFPSItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	OnUnhovered.ExecuteIfBound(ItemRowId);
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
	if (TB_Count)
	{
		TB_Count->SetVisibility(ESlateVisibility::Hidden);
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
