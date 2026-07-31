#include "UI/Common/LastFPSNoticeWidget.h"

#include "UI/Framework/LastFPSButtonBase.h"

void ULastFPSNoticeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Ok)
	{
		Button_Ok->OnClicked().AddUObject(this, &ULastFPSNoticeWidget::HandleOkClicked);
	}
}

bool ULastFPSNoticeWidget::NativeOnHandleBackAction()
{
	HandleOkClicked();
	return true;
}

void ULastFPSNoticeWidget::SetupNotice(const FText& InTitle, const FText& InBody)
{
	UCommonGameDialogDescriptor* Descriptor =
		UCommonGameDialogDescriptor::CreateConfirmationOk(InTitle, InBody);
	SetupDialog(Descriptor, FCommonMessagingResultDelegate());
}

void ULastFPSNoticeWidget::SetupDialog(
	UCommonGameDialogDescriptor* Descriptor,
	FCommonMessagingResultDelegate ResultCallback)
{
	OnNoticeClosed.Clear();
	CloseResult = ECommonMessagingResult::Confirmed;
	if (Descriptor && Button_Ok && Descriptor->ButtonActions.IsValidIndex(0))
	{
		const FConfirmationDialogAction& CloseAction =
			Descriptor->ButtonActions[0];
		CloseResult = CloseAction.Result;
		const FText& DisplayText = CloseAction.OptionalDisplayText;
		if (!DisplayText.IsEmpty())
		{
			Button_Ok->SetButtonText(DisplayText);
		}
	}

	Super::SetupDialog(Descriptor, MoveTemp(ResultCallback));
}

void ULastFPSNoticeWidget::KillDialog()
{
	CompleteDialog(ECommonMessagingResult::Killed);
	OnNoticeClosed.Broadcast();
	DeactivateWithAnimation();
}

void ULastFPSNoticeWidget::HandleOkClicked()
{
	CloseNotice();
}

void ULastFPSNoticeWidget::CloseNotice()
{
	CompleteDialog(CloseResult);
	OnNoticeClosed.Broadcast();
	DeactivateWithAnimation();
}
