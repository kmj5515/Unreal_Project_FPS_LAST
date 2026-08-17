#include "UI/Equipment/LastFPSEquipmentSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Framework/LastFPSIconLoader.h"
#include "UI/Theme/LastFPSUITheme.h"
#include "UI/Theme/LastFPSUIThemeAsset.h"

void ULastFPSEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Slot)
	{
		Button_Slot->OnClicked().AddUObject(this, &ULastFPSEquipmentSlotWidget::HandleSlotClicked);
	}
}

void ULastFPSEquipmentSlotWidget::NativeDestruct()
{
	// 콜백은 약참조라 안전하지만, 쓰지 않을 로드를 계속 진행시킬 이유가 없다.
	LastFPSIconLoader::CancelRequest(IconLoadHandle);

	Super::NativeDestruct();
}

void ULastFPSEquipmentSlotWidget::InitializeSlot(
	const ELastFPSEquipmentSlotType InSlotType, const int32 InSlotIndex, const FText& InSlotLabel)
{
	SlotType  = InSlotType;
	SlotIndex = InSlotIndex;

	if (TB_SlotLabel)
	{
		TB_SlotLabel->SetText(InSlotLabel);
	}
}

void ULastFPSEquipmentSlotWidget::SetEquipped(const FLastFPSItemData& InItem, const FName InRowId)
{
	ItemRowId = InRowId;

	if (Image_Icon)
	{
		LastFPSIconLoader::CancelRequest(IconLoadHandle);

		const bool bHasIconSource =
			!InItem.Icon.IsNull()
			|| (InItem.ItemType == ELastFPSItemType::Weapon && !InItem.WeaponDefinition.IsNull());

		if (bHasIconSource)
		{
			Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);

			if (InItem.ItemType == ELastFPSItemType::Weapon && !InItem.WeaponDefinition.IsNull())
			{
				// 장비 무기 슬롯은 무기 정의의 범용 Icon(캡처 렌더)을 쓴다. HUD 슬롯은 별도의
				// HUDWeaponSlotIcon 을 쓰므로 여기서 그쪽을 참조하지 않는다. 아이콘 경로는 무기 정의만 알고 있어
				// 정의를 먼저 받아야 하는데, 정의가 SkeletalMesh 등을 하드 참조해 동기 로드하면
				// 화면을 여는 프레임이 슬롯 수만큼 멈춘다.
				IconLoadHandle = LastFPSIconLoader::RequestWeaponIcon(
					*this, *Image_Icon, InItem.WeaponDefinition, InItem.Icon);
			}
			else
			{
				IconLoadHandle = LastFPSIconLoader::RequestIcon(*this, *Image_Icon, InItem.Icon);
			}
		}
		else
		{
			Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	EquippedRarity = InItem.Rarity;
	RefreshRarityVisual();

	if (Img_Empty)
	{
		Img_Empty->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TB_ItemName)
	{
		TB_ItemName->SetText(InItem.ItemName);
	}
}

void ULastFPSEquipmentSlotWidget::SetEmpty()
{
	ItemRowId = NAME_None;

	// 빈 슬롯이 되었으니 이전 아이템의 아이콘이 뒤늦게 도착해 덮어쓰지 않게 끊는다.
	LastFPSIconLoader::CancelRequest(IconLoadHandle);

	if (Image_Icon)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Img_RarityBorder)
	{
		Img_RarityBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Img_Empty)
	{
		Img_Empty->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (TB_ItemName)
	{
		TB_ItemName->SetText(FText::GetEmpty());
	}
}

void ULastFPSEquipmentSlotWidget::RefreshRarityVisual()
{
	if (!Img_RarityBorder || ItemRowId.IsNone())
	{
		return;
	}

	LastFPSUITheme::ApplyRarityGlow(
		*Img_RarityBorder, LastFPSGetRarityColor(EquippedRarity), RarityGlowIntensity);
	Img_RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULastFPSEquipmentSlotWidget::ApplyUITheme(const ULastFPSUIThemeAsset& Theme)
{
	RarityGlowIntensity = Theme.Surface.RarityGlowIntensity;

	if (TB_SlotLabel)
	{
		TB_SlotLabel->SetFont(Theme.Typography.Caption);
		TB_SlotLabel->SetColorAndOpacity(FSlateColor(Theme.Palette.TextMuted));
	}

	if (TB_ItemName)
	{
		TB_ItemName->SetFont(Theme.Typography.Label);
		TB_ItemName->SetColorAndOpacity(FSlateColor(Theme.Palette.TextPrimary));
	}

	RefreshRarityVisual();
}

void ULastFPSEquipmentSlotWidget::HandleSlotClicked()
{
	OnSlotClicked.ExecuteIfBound(SlotType, SlotIndex);
}

FReply ULastFPSEquipmentSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 우클릭은 즉시 해제. 좌클릭은 Button_Slot 이 받아 선택 패널을 여는 경로와 분리한다.
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !ItemRowId.IsNone())
	{
		OnSlotUnequipRequested.ExecuteIfBound(SlotType, SlotIndex);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
