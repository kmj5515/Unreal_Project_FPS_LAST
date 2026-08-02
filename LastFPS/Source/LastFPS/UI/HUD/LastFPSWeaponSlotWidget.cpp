#include "UI/HUD/LastFPSWeaponSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Engine/Texture2D.h"

void ULastFPSWeaponSlotWidget::SetupSlot(
	const int32 SlotIndex, const ULastFPSWeaponDefinition* Definition, const bool bIsActive)
{
	if (TB_SlotKey)
	{
		// 1·2 키와 눈으로 바로 맞아떨어지도록 1-based 로 표시한다.
		TB_SlotKey->SetText(FText::AsNumber(SlotIndex + 1));
	}

	if (TB_WeaponName)
	{
		TB_WeaponName->SetText(Definition ? Definition->DisplayName : EmptySlotText);
	}

	if (Img_WeaponIcon)
	{
		// HUD 는 이미 표시 중이라 지연 로드를 기다릴 수 없다. 아이콘은 작은 UI 텍스처이므로 동기 로드한다.
		UTexture2D* IconTexture = Definition ? Definition->Icon.LoadSynchronous() : nullptr;
		if (IconTexture)
		{
			Img_WeaponIcon->SetBrushFromTexture(IconTexture);
			Img_WeaponIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Img_WeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	SetActive(bIsActive);
}

void ULastFPSWeaponSlotWidget::SetActive(const bool bIsActive)
{
	if (Img_ActiveHighlight)
	{
		Img_ActiveHighlight->SetVisibility(
			bIsActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	SetRenderOpacity(bIsActive ? 1.f : FMath::Clamp(InactiveOpacity, 0.f, 1.f));
}
