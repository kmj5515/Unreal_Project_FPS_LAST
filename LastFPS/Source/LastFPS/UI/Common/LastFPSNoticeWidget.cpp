#include "UI/Common/LastFPSNoticeWidget.h"

#include "Data/Definitions/LastFPSPopupCatalog.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Framework/LastFPSPopupSubsystem.h"
#include "UI/Framework/LastFPSPopupTags.h"

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


ULastFPSNoticeWidget* ULastFPSNoticeWidget::ShowPopup(const UObject* WorldContext, const FLastFPSNoticeParams& Params)
{
	ULastFPSNoticeWidget* Widget = ULastFPSPopupSubsystem::ShowPopup<ULastFPSNoticeWidget>(WorldContext, LastFPSPopupTags::Notice());
	if (Widget)
	{
		Widget->SetupNotice(Params.Title, Params.Body, Params.OnResult);
	}
	
	return Widget;
}

void ULastFPSNoticeWidget::SetupNotice(const FText& InTitle, const FText& InBody, FCommonMessagingResultDelegate ResultCallback)
{
	UCommonGameDialogDescriptor* Descriptor =
		UCommonGameDialogDescriptor::CreateConfirmationOk(InTitle, InBody);
	SetupDialog(Descriptor, ResultCallback);
}

void ULastFPSNoticeWidget::SetupDialog(
	UCommonGameDialogDescriptor* Descriptor,
	FCommonMessagingResultDelegate ResultCallback)
{
	CloseResult = ECommonMessagingResult::Confirmed;
	if (Descriptor && Button_Ok && Descriptor->ButtonActions.IsValidIndex(0))
	{
		const FConfirmationDialogAction& CloseAction = Descriptor->ButtonActions[0];
		CloseResult = CloseAction.Result;
		const FText& DisplayText = CloseAction.OptionalDisplayText;
		if (!DisplayText.IsEmpty())
		{
			Button_Ok->SetButtonText(DisplayText);
		}
	}

	Super::SetupDialog(Descriptor,ResultCallback );
}

void ULastFPSNoticeWidget::KillDialog()
{
	CompleteDialog(ECommonMessagingResult::Killed);
	DeactivateWithAnimation();
}

void ULastFPSNoticeWidget::HandleOkClicked()
{
	CloseNotice();
}

void ULastFPSNoticeWidget::CloseNotice()
{
	CompleteDialog(CloseResult);
	DeactivateWithAnimation();
}
