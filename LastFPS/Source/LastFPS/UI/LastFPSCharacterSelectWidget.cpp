#include "UI/LastFPSCharacterSelectWidget.h"

#include "UI/LastFPSCharacterCardWidget.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void ULastFPSCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.AddDynamic(this, &ULastFPSCharacterSelectWidget::HandleConfirmClicked);
	}
	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &ULastFPSCharacterSelectWidget::HandleBackClicked);
	}
	if (Button_Prev)
	{
		Button_Prev->OnClicked.AddDynamic(this, &ULastFPSCharacterSelectWidget::HandlePrevClicked);
	}
	if (Button_Next)
	{
		Button_Next->OnClicked.AddDynamic(this, &ULastFPSCharacterSelectWidget::HandleNextClicked);
	}

	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		OnSelectionChanged(PC->GetSelectedCharacterIndex(), PC->GetSelectableCharacterClasses().Num());
	}
}

void ULastFPSCharacterSelectWidget::OnSelectionChanged_Implementation(int32 NewIndex, int32 TotalCount)
{
	TObjectPtr<ULastFPSCharacterCardWidget> Cards[] = { Card_0, Card_1, Card_2 };
	for (int32 i = 0; i < 3; ++i)
	{
		if (Cards[i])
		{
			Cards[i]->SetSelected(i == NewIndex);
		}
	}

	if (TB_CharName)
	{
		TB_CharName->SetText(
			CharacterNames.IsValidIndex(NewIndex) ? CharacterNames[NewIndex] : FText::GetEmpty());
	}
	if (TB_CharRole)
	{
		TB_CharRole->SetText(
			CharacterRoles.IsValidIndex(NewIndex) ? CharacterRoles[NewIndex] : FText::GetEmpty());
	}

	if (Button_Prev)
	{
		Button_Prev->SetIsEnabled(NewIndex > 0);
	}
	if (Button_Next)
	{
		Button_Next->SetIsEnabled(NewIndex < TotalCount - 1);
	}
}

void ULastFPSCharacterSelectWidget::HandleConfirmClicked()
{
	if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
	{
		GI->RequestTravelToHub();
	}
}

void ULastFPSCharacterSelectWidget::HandleBackClicked()
{
	if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
	{
		GI->RequestTravelToMainMenu();
	}
}

void ULastFPSCharacterSelectWidget::HandlePrevClicked()
{
	ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>();
	if (!PC)
	{
		return;
	}

	PC->SetSelectedCharacterIndex(PC->GetSelectedCharacterIndex() - 1);
	OnSelectionChanged(PC->GetSelectedCharacterIndex(), PC->GetSelectableCharacterClasses().Num());
}

void ULastFPSCharacterSelectWidget::HandleNextClicked()
{
	ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>();
	if (!PC)
	{
		return;
	}

	PC->SetSelectedCharacterIndex(PC->GetSelectedCharacterIndex() + 1);
	OnSelectionChanged(PC->GetSelectedCharacterIndex(), PC->GetSelectableCharacterClasses().Num());
}
