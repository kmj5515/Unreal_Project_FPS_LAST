#include "UI/Framework/LastFPSButtonBase.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Sound/SoundBase.h"

void ULastFPSButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (TextBlock)
	{
		TextBlock->SetText(ButtonText);
	}

	if (SizeBox)
	{
		if (bOverrideSize)
		{
			SizeBox->SetWidthOverride(ButtonSize.X);
			SizeBox->SetHeightOverride(ButtonSize.Y);
		}
		else
		{
			SizeBox->ClearWidthOverride();
			SizeBox->ClearHeightOverride();
		}
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
