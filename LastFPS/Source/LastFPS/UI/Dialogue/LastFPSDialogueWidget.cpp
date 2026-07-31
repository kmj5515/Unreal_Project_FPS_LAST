#include "UI/Dialogue/LastFPSDialogueWidget.h"

#include "UI/Framework/LastFPSButtonBase.h"
#include "Components/TextBlock.h"

void ULastFPSDialogueWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Next)
	{
		Button_Next->OnClicked().AddUObject(this, &ULastFPSDialogueWidget::HandleNextClicked);
	}
}

bool ULastFPSDialogueWidget::NativeOnHandleBackAction()
{
	// Back/ESC → 대화 전체 닫기 (페이지 스킵 아님).
	CloseDialogue();
	return true;
}

void ULastFPSDialogueWidget::SetupDialogue(const FText& InSpeaker, const TArray<FText>& InLines)
{
	Speaker = InSpeaker;
	Lines = InLines;
	CurrentIndex = 0;

	if (Lines.Num() == 0)
	{
		CloseDialogue();
		return;
	}

	ShowCurrentLine();
}

void ULastFPSDialogueWidget::KillDialog()
{
	CompleteDialog(ECommonMessagingResult::Killed);
	CloseDialogue();
}

void ULastFPSDialogueWidget::HandleNextClicked()
{
	if (CurrentIndex + 1 < Lines.Num())
	{
		++CurrentIndex;
		ShowCurrentLine();
	}
	else
	{
		CloseDialogue();
	}
}

void ULastFPSDialogueWidget::ShowCurrentLine()
{
	if (!Lines.IsValidIndex(CurrentIndex))
	{
		return;
	}

	SetDialogText(Speaker, Lines[CurrentIndex]);

	if (Text_NextLabel)
	{
		const bool bLastPage = (CurrentIndex + 1 >= Lines.Num());
		Text_NextLabel->SetText(bLastPage ? CloseLabel : NextLabel);
	}
}

void ULastFPSDialogueWidget::CloseDialogue()
{
	DeactivateWithAnimation();
}
