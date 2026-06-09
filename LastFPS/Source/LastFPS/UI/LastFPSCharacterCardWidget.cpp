#include "UI/LastFPSCharacterCardWidget.h"

#include "Game/LastFPSCharacterDefinition.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"

void ULastFPSCharacterCardWidget::SetupCard_Implementation(const ULastFPSCharacterDefinition* Def)
{
	if (TB_CardName)
	{
		TB_CardName->SetText(Def ? Def->DisplayName : FText::GetEmpty());
	}
	if (TB_CardRole)
	{
		TB_CardRole->SetText(Def ? Def->Role : FText::GetEmpty());
	}
}

FReply ULastFPSCharacterCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCardClicked.ExecuteIfBound(CardIndex);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
