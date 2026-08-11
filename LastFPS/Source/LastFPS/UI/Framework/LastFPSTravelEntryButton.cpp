#include "UI/Framework/LastFPSTravelEntryButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSTravelEntryButton)

void ULastFPSTravelEntryButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLockedVisual();
	RefreshLockedVisual();
}

void ULastFPSTravelEntryButton::ApplyTravelAccess(
	const bool bUnlocked,
	TSoftObjectPtr<UTexture2D> LockedIcon)
{
	bTravelLocked = !bUnlocked;
	LockedIconReference = MoveTemp(LockedIcon);
	SetIsEnabled(bUnlocked);
	EnsureLockedVisual();
	RefreshLockedVisual();
	OnTravelLockStateChanged(bTravelLocked, LockedIconReference);
}

void ULastFPSTravelEntryButton::EnsureLockedVisual()
{
	if (LockedImage || !WidgetTree || !WidgetTree->RootWidget)
	{
		return;
	}

	UWidget* ExistingRoot = WidgetTree->RootWidget;
	UOverlay* LockOverlay = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(), TEXT("TravelLockOverlay"));
	USizeBox* LockSizeBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("TravelLockSize"));
	LockedImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("TravelLockImage"));
	if (!LockOverlay || !LockSizeBox || !LockedImage)
	{
		LockedImage = nullptr;
		return;
	}

	// 기존 WBP 전체를 한 겹 감싸므로 내부 레이아웃과 입력 계약은 그대로 유지된다.
	WidgetTree->RootWidget = LockOverlay;
	if (UOverlaySlot* ContentSlot = LockOverlay->AddChildToOverlay(ExistingRoot))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
	}

	LockSizeBox->SetWidthOverride(LockedIconSize);
	LockSizeBox->SetHeightOverride(LockedIconSize);
	LockSizeBox->AddChild(LockedImage);
	if (UOverlaySlot* LockSlot = LockOverlay->AddChildToOverlay(LockSizeBox))
	{
		LockSlot->SetHorizontalAlignment(HAlign_Center);
		LockSlot->SetVerticalAlignment(VAlign_Center);
	}
	LockedImage->SetVisibility(ESlateVisibility::Collapsed);
}

void ULastFPSTravelEntryButton::RefreshLockedVisual()
{
	if (!LockedImage)
	{
		return;
	}

	if (!LockedIconReference.IsNull())
	{
		// UImage가 소프트 텍스처의 비동기 스트리밍 수명과 브러시 갱신을 소유한다.
		LockedImage->SetBrushFromSoftTexture(LockedIconReference, false);
	}
	LockedImage->SetVisibility(
		bTravelLocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
