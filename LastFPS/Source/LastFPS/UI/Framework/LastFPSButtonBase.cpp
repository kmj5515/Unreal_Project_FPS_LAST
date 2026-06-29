#include "UI/Framework/LastFPSButtonBase.h"

#include "Components/TextBlock.h"

void ULastFPSButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (TextBlock)
	{
		TextBlock->SetText(ButtonText);
	}
}

void ULastFPSButtonBase::SetButtonText(const FText& InText)
{
	ButtonText = InText;

	if (TextBlock)
	{
		TextBlock->SetText(InText);
	}
}
