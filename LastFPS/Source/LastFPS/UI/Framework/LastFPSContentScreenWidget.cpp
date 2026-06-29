#include "UI/Framework/LastFPSContentScreenWidget.h"

#include "UI/Framework/LastFPSButtonBase.h"
#include "Components/TextBlock.h"

void ULastFPSContentScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TB_Title)
	{
		TB_Title->SetText(ScreenTitle);
	}

	if (Button_Close)
	{
		Button_Close->OnClicked().AddUObject(this, &ULastFPSContentScreenWidget::HandleCloseClicked);
	}
}

void ULastFPSContentScreenWidget::HandleCloseClicked()
{
	// 레이어 스택에서 자신을 pop. Back 액션과 동일 결과.
	DeactivateWidget();
}
