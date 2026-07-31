#include "UI/Loading/LastFPSLoadingIndicatorWidget.h"

#include "Input/CommonUIInputTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSLoadingIndicatorWidget)

ULastFPSLoadingIndicatorWidget::ULastFPSLoadingIndicatorWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSetVisibilityOnActivated = true;
	ActivatedVisibility = ESlateVisibility::Visible;
	bSetVisibilityOnDeactivated = true;
	DeactivatedVisibility = ESlateVisibility::Collapsed;
	bSupportsActivationFocus = true;
	bIsModal = true;
	bIsBackHandler = false;
}

void ULastFPSLoadingIndicatorWidget::SetStatusText(const FText& InStatusText)
{
	StatusText = InStatusText;
}

TOptional<FUIInputConfig> ULastFPSLoadingIndicatorWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
