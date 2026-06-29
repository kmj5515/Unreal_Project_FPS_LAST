#include "UI/Framework/LastFPSModalDialogBase.h"

#include "Components/TextBlock.h"

void ULastFPSModalDialogBase::SetDialogText(const FText& InTitle, const FText& InBody)
{
	if (Text_Title)
	{
		Text_Title->SetText(InTitle);
	}
	if (Text_Body)
	{
		Text_Body->SetText(InBody);
	}
}
