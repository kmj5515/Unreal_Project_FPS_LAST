#include "UI/LastFPSActivatableWidget.h"

ULastFPSActivatableWidget::ULastFPSActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

bool ULastFPSActivatableWidget::NativeOnHandleBackAction()
{
	return Super::NativeOnHandleBackAction();
}
