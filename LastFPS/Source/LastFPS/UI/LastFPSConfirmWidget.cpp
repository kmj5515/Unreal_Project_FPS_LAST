#include "UI/LastFPSConfirmWidget.h"

#include "Components/Button.h"

void ULastFPSConfirmWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.AddDynamic(this, &ULastFPSConfirmWidget::HandleConfirmClicked);
	}
	if (Button_Cancel)
	{
		Button_Cancel->OnClicked.AddDynamic(this, &ULastFPSConfirmWidget::HandleCancelClicked);
	}
}

bool ULastFPSConfirmWidget::NativeOnHandleBackAction()
{
	HandleCancelClicked();
	return true;
}

void ULastFPSConfirmWidget::SetupConfirm(const FText& InTitle, const FText& InBody)
{
	SetDialogText(InTitle, InBody);
}

void ULastFPSConfirmWidget::HandleConfirmClicked()
{
	CloseWithResult(true);
}

void ULastFPSConfirmWidget::HandleCancelClicked()
{
	CloseWithResult(false);
}

void ULastFPSConfirmWidget::CloseWithResult(const bool bConfirmed)
{
	OnConfirmResult.Broadcast(bConfirmed);
	DeactivateWidget();
}
