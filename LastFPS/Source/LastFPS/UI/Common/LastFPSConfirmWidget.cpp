#include "UI/Common/LastFPSConfirmWidget.h"

#include "UI/Framework/LastFPSButtonBase.h"

void ULastFPSConfirmWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked().AddUObject(this, &ULastFPSConfirmWidget::HandleConfirmClicked);
	}
	if (Button_Cancel)
	{
		Button_Cancel->OnClicked().AddUObject(this, &ULastFPSConfirmWidget::HandleCancelClicked);
	}
}

bool ULastFPSConfirmWidget::NativeOnHandleBackAction()
{
	CloseWithMessagingResult(false, BackResult);
	return true;
}

void ULastFPSConfirmWidget::SetupConfirm(const FText& InTitle, const FText& InBody)
{
	UCommonGameDialogDescriptor* Descriptor =
		UCommonGameDialogDescriptor::CreateConfirmationYesNo(InTitle, InBody);
	SetupDialog(Descriptor, FCommonMessagingResultDelegate());
}

void ULastFPSConfirmWidget::SetupDialog(
	UCommonGameDialogDescriptor* Descriptor,
	FCommonMessagingResultDelegate ResultCallback)
{
	OnConfirmResult.Clear();
	ConfirmResult = ECommonMessagingResult::Confirmed;
	CancelResult = ECommonMessagingResult::Declined;
	BackResult = ECommonMessagingResult::Cancelled;

	if (Descriptor)
	{
		if (Descriptor->ButtonActions.IsValidIndex(0))
		{
			const FConfirmationDialogAction& ConfirmAction =
				Descriptor->ButtonActions[0];
			ConfirmResult = ConfirmAction.Result;
			if (Button_Confirm && !ConfirmAction.OptionalDisplayText.IsEmpty())
			{
				Button_Confirm->SetButtonText(
					ConfirmAction.OptionalDisplayText);
			}
		}

		const bool bHasCancelAction = Descriptor->ButtonActions.IsValidIndex(1);
		if (Button_Cancel)
		{
			Button_Cancel->SetVisibility(
				bHasCancelAction
					? ESlateVisibility::Visible
					: ESlateVisibility::Collapsed);
		}

		if (bHasCancelAction)
		{
			const FConfirmationDialogAction& CancelAction =
				Descriptor->ButtonActions[1];
			CancelResult = CancelAction.Result;
			BackResult = CancelResult;
			if (Button_Cancel && !CancelAction.OptionalDisplayText.IsEmpty())
			{
				Button_Cancel->SetButtonText(
					CancelAction.OptionalDisplayText);
			}
		}

		if (Descriptor->ButtonActions.IsValidIndex(2))
		{
			BackResult = Descriptor->ButtonActions[2].Result;
		}
	}

	Super::SetupDialog(Descriptor, MoveTemp(ResultCallback));
}

void ULastFPSConfirmWidget::KillDialog()
{
	CloseWithMessagingResult(false, ECommonMessagingResult::Killed);
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
	CloseWithMessagingResult(
		bConfirmed,
		bConfirmed ? ConfirmResult : CancelResult);
}

void ULastFPSConfirmWidget::CloseWithMessagingResult(
	const bool bConfirmed,
	const ECommonMessagingResult Result)
{
	CompleteDialog(Result);
	OnConfirmResult.Broadcast(bConfirmed);
	DeactivateWidget();
}
