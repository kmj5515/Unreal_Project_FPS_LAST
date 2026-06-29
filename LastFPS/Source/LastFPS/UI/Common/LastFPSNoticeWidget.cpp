#include "UI/Common/LastFPSNoticeWidget.h"

#include "UI/Framework/LastFPSButtonBase.h"

void ULastFPSNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

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
	SetDialogText(InTitle, InBody);
}

void ULastFPSNoticeWidget::HandleOkClicked()
{
	CloseNotice();
}

void ULastFPSNoticeWidget::CloseNotice()
{
	OnNoticeClosed.Broadcast();
	DeactivateWidget();
}
