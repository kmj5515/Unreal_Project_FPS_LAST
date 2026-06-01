#include "UI/LastFPSNoticeWidget.h"

#include "Components/Button.h"

void ULastFPSNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Ok)
	{
		Button_Ok->OnClicked.AddDynamic(this, &ULastFPSNoticeWidget::HandleOkClicked);
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
