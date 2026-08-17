#include "UI/HUD/LastFPSWeaponSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "UI/Framework/LastFPSIconLoader.h"

void ULastFPSWeaponSlotWidget::NativeDestruct()
{
	LastFPSIconLoader::CancelRequest(IconLoadHandle);

	Super::NativeDestruct();
}

void ULastFPSWeaponSlotWidget::SetupSlot(
	const int32 SlotIndex, const ULastFPSWeaponDefinition* Definition, const bool bIsActive)
{
	if (TB_SlotKey)
	{
		// 1·2 키와 눈으로 바로 맞아떨어지도록 1-based 로 표시한다.
		TB_SlotKey->SetText(FText::AsNumber(SlotIndex + 1));
	}

	// if (TB_WeaponName)
	// {
	// 	TB_WeaponName->SetText(Definition ? Definition->DisplayName : EmptySlotText);
	// }

	if (Img_WeaponIcon)
	{
		LastFPSIconLoader::CancelRequest(IconLoadHandle);

		// HUD 전용 아이콘이 있으면 그것을, 없으면 범용 Icon 을 쓴다(무기 정의에 기술된 대체 규칙).
		const TSoftObjectPtr<UTexture2D>* Icon = nullptr;
		if (Definition)
		{
			Icon = Definition->HUDWeaponSlotIcon.IsNull() ? &Definition->Icon : &Definition->HUDWeaponSlotIcon;
		}

		if (Icon && !Icon->IsNull())
		{
			// 재사용된 슬롯에서 이전 브러시가 비동기 로드 동안 노출되지 않도록 먼저 비운다.
			Img_WeaponIcon->SetBrushFromTexture(nullptr);
			Img_WeaponIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
			IconLoadHandle = LastFPSIconLoader::RequestIcon(*this, *Img_WeaponIcon, *Icon);
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
