#include "UI/LastFPSActivatableWidget.h"

ULastFPSActivatableWidget::ULastFPSActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

bool ULastFPSActivatableWidget::NativeOnHandleBackAction()
{
	return Super::NativeOnHandleBackAction();
}

void ULastFPSActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 키 입력(ESC)을 받으려면 포커스가 필요. CommonUI Back이 미구성이라 직접 확보.
	SetIsFocusable(true);
	SetKeyboardFocus();
}

FReply ULastFPSActivatableWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bIsBackHandler && InKeyEvent.GetKey() == EKeys::Escape)
	{
		DeactivateWidget();   // 레이어 스택에서 pop → 입력모드 복귀
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
