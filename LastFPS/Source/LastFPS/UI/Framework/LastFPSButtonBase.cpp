#include "UI/Framework/LastFPSButtonBase.h"

#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void ULastFPSButtonBase::NativeOnClicked()
{
	if (PressedSound)
	{
		UGameplayStatics::PlaySound2D(this, PressedSound);
	}

	Super::NativeOnClicked();
}

void ULastFPSButtonBase::NativeOnHovered()
{
	if (HoveredSound)
	{
		UGameplayStatics::PlaySound2D(this, HoveredSound);
	}

	Super::NativeOnHovered();
}

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
