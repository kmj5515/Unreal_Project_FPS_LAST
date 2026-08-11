#include "UI/GameMenu/LastFPSGameMenuWidget.h"

#include "Game/LastFPSPlayerController.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Framework/LastFPSUITags.h"

void ULastFPSGameMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Inventory)
	{
		Button_Inventory->OnClicked().AddUObject(this, &ULastFPSGameMenuWidget::HandleInventoryClicked);
	}
	if (Button_Settings)
	{
		Button_Settings->OnClicked().AddUObject(this, &ULastFPSGameMenuWidget::HandleSettingsClicked);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked().AddUObject(this, &ULastFPSGameMenuWidget::HandleQuitClicked);
	}
}

void ULastFPSGameMenuWidget::NativeDestruct()
{
	if (Button_Inventory)
	{
		Button_Inventory->OnClicked().RemoveAll(this);
	}
	if (Button_Settings)
	{
		Button_Settings->OnClicked().RemoveAll(this);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

UWidget* ULastFPSGameMenuWidget::NativeGetDesiredFocusTarget() const
{
	return Button_Inventory;
}

void ULastFPSGameMenuWidget::HandleInventoryClicked()
{
	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		DeactivateWidget();
		PC->OpenScreen(LastFPSUITags::Screen_Inventory());
	}
}

void ULastFPSGameMenuWidget::HandleSettingsClicked()
{
	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		DeactivateWidget();
		PC->OpenScreen(LastFPSUITags::Screen_Settings());
	}
}

void ULastFPSGameMenuWidget::HandleQuitClicked()
{
	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		PC->RequestQuitGame();
	}
}
