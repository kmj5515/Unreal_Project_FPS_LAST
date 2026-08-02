#include "UI/Inventory/LastFPSItemSlotWidget.h"

#include "Localization/LastFPSLocalization.h"
#include "UI/Framework/LastFPSIconLoader.h"
#include "UI/Inventory/LastFPSItemTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

void ULastFPSItemSlotWidget::NativeDestruct()
{
	LastFPSIconLoader::CancelRequest(IconLoadHandle);

	Super::NativeDestruct();
}

UWidget* ULastFPSItemSlotWidget::CreateItemTooltip()
{
	if (!TooltipWidgetClass)
	{
		return nullptr;
	}

	ULastFPSItemTooltipWidget* Tooltip = CreateWidget<ULastFPSItemTooltipWidget>(this, TooltipWidgetClass);
	if (!Tooltip)
	{
		return nullptr;
	}

	Tooltip->SetupTooltip(CachedItem, ItemRowId);
	return Tooltip;
}

void ULastFPSItemSlotWidget::SetupSlot(const FLastFPSItemData& InItem, FName InRowId, int32 Count)
{
	ItemRowId = InRowId;
	CachedItem = InItem;

	// hover(마우스 진입/이탈) 이벤트와 툴팁이 동작하려면 슬롯 루트가 히트테스트 가능해야 한다.
	SetVisibility(ESlateVisibility::Visible);

	// 툴팁은 호버할 때만 만든다. 여기서 미리 만들면 아이템 수만큼 위젯이 생겨 인벤토리를 여는 순간
	// 그대로 비용이 된다. 델리게이트만 걸어 두면 UMG 가 필요할 때 한 번 호출한다.
	if (TooltipWidgetClass && !ToolTipWidgetDelegate.IsBound())
	{
		ToolTipWidgetDelegate.BindDynamic(this, &ULastFPSItemSlotWidget::CreateItemTooltip);
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

	// 아이콘은 비동기로 받는다. 슬롯마다 동기 로드하면 아이템 수만큼 블로킹 IO 가 쌓인다.
	if (Image_Icon)
	{
		LastFPSIconLoader::CancelRequest(IconLoadHandle);
		IconLoadHandle = LastFPSIconLoader::RequestIcon(*this, *Image_Icon, InItem.Icon);
		Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (TB_ItemName)
	{
		TB_ItemName->SetText(InItem.ItemName);
	}

	if (TB_Rarity)
	{
		TB_Rarity->SetText(FLastFPSLocalization::GetUIEnumText(
			StaticEnum<ELastFPSItemRarity>(),
			static_cast<int64>(InItem.Rarity)));
		TB_Rarity->SetColorAndOpacity(FSlateColor(RarityToColor(InItem.Rarity)));
	}

	if (TB_Count)
	{
		if (Count > 1)
		{
			TB_Count->SetText(FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::CountFormat),
				FText::AsNumber(Count)));
			TB_Count->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			TB_Count->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	SetSelected(false);
}

void ULastFPSItemSlotWidget::SetSelected(const bool bInSelected)
{
	if (Img_SelectionBorder)
	{
		Img_SelectionBorder->SetVisibility(
			bInSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

FReply ULastFPSItemSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& !ItemRowId.IsNone()
		&& OnClicked.IsBound())
	{
		OnClicked.Execute(ItemRowId);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULastFPSItemSlotWidget::NativeOnMouseButtonDoubleClick(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& !ItemRowId.IsNone()
		&& OnDoubleClicked.IsBound())
	{
		OnDoubleClicked.Execute(ItemRowId);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
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
	ItemRowId = NAME_None;
	CachedItem = FLastFPSItemData();
	SetToolTip(nullptr);
	// 델리게이트를 남겨 두면 빈 슬롯에도 툴팁이 뜬다.
	ToolTipWidgetDelegate.Unbind();
	SetSelected(false);

	// 이전 아이템의 아이콘이 뒤늦게 도착해 빈 슬롯에 그려지지 않게 끊는다.
	LastFPSIconLoader::CancelRequest(IconLoadHandle);

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
	return LastFPSGetRarityColor(Rarity);
}
