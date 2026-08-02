#include "UI/Common/LastFPSConfirmWidget.h"

#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Framework/LastFPSPopupSubsystem.h"
#include "UI/Framework/LastFPSPopupTags.h"

ULastFPSConfirmWidget* ULastFPSConfirmWidget::ShowPopup(
	const UObject* WorldContext, const FLastFPSConfirmParams& Params)
{
	ULastFPSConfirmWidget* Widget = ULastFPSPopupSubsystem::ShowPopup<ULastFPSConfirmWidget>(
		WorldContext, LastFPSPopupTags::Confirmation());
	if (Widget)
	{
		Widget->ApplyParams(Params);
	}
	return Widget;
}

void ULastFPSConfirmWidget::ShowAsyncPopup(
	const UObject* WorldContext, const FLastFPSConfirmParams& Params)
{
	// 로드가 끝난 뒤 적용해야 하므로 파라미터를 값으로 캡처한다(호출부의 지역 변수가 먼저 사라질 수 있다).
	ULastFPSPopupSubsystem::ShowPopupAsync<ULastFPSConfirmWidget>(
		WorldContext,
		LastFPSPopupTags::Confirmation(),
		[Params](ULastFPSConfirmWidget* Widget)
		{
			if (Widget)
			{
				Widget->ApplyParams(Params);
				return;
			}

			// 팝업을 못 열어도 결과를 기다리는 쪽이 멈추지 않도록 실패를 통보한다.
			Params.OnResult.ExecuteIfBound(ECommonMessagingResult::Unknown);
		});
}

void ULastFPSConfirmWidget::ApplyParams(const FLastFPSConfirmParams& Params)
{
	// SetupConfirm 은 결과 콜백을 빈 값으로 넘겨 Params.OnResult 가 버려지므로 SetupDialog 를 직접 부른다.
	UCommonGameDialogDescriptor* Descriptor =
		UCommonGameDialogDescriptor::CreateConfirmationYesNo(Params.Title, Params.Body);
	SetupDialog(Descriptor, Params.OnResult);
}

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
	DeactivateWithAnimation();
}
