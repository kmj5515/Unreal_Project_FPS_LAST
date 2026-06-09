#include "UI/LastFPSCharacterCardWidget.h"

#include "Input/Events.h"

FReply ULastFPSCharacterCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCardClicked.ExecuteIfBound(CardIndex);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
