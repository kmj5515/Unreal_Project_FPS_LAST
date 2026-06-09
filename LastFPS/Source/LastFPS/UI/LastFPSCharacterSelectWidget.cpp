#include "UI/LastFPSCharacterSelectWidget.h"

#include "UI/LastFPSCharacterCardWidget.h"
#include "UI/LastFPSButtonBase.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"
#include "Game/LastFPSCharacterDefinition.h"

#include "Components/TextBlock.h"

void ULastFPSCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked().AddUObject(this, &ULastFPSCharacterSelectWidget::HandleConfirmClicked);
	}
	if (Button_Back)
	{
		Button_Back->OnClicked().AddUObject(this, &ULastFPSCharacterSelectWidget::HandleBackClicked);
	}
	if (Button_Prev)
	{
		Button_Prev->OnClicked().AddUObject(this, &ULastFPSCharacterSelectWidget::HandlePrevClicked);
	}
	if (Button_Next)
	{
		Button_Next->OnClicked().AddUObject(this, &ULastFPSCharacterSelectWidget::HandleNextClicked);
	}

	TObjectPtr<ULastFPSCharacterCardWidget> Cards[] = { Card_0, Card_1, Card_2 };
	for (int32 i = 0; i < 3; ++i)
	{
		if (Cards[i])
		{
			Cards[i]->CardIndex = i;
			Cards[i]->OnCardClicked.BindUObject(this, &ULastFPSCharacterSelectWidget::HandleCardClicked);

			const ULastFPSCharacterDefinition* Def = CharacterDefinitions.IsValidIndex(i) ? CharacterDefinitions[i] : nullptr;
			Cards[i]->SetupCard(Def);
		}
	}

	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		OnSelectionChanged(PC->GetSelectedCharacterIndex(), CharacterDefinitions.Num());
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

	const ULastFPSCharacterDefinition* Def =
		CharacterDefinitions.IsValidIndex(NewIndex) ? CharacterDefinitions[NewIndex] : nullptr;

	if (TB_CharName)
	{
		TB_CharName->SetText(Def ? Def->DisplayName : FText::GetEmpty());
	}
	if (TB_CharRole)
	{
		TB_CharRole->SetText(Def ? Def->Role : FText::GetEmpty());
	}
	if (TB_CharDesc)
	{
		TB_CharDesc->SetText(Def ? Def->Description : FText::GetEmpty());
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
	OnSelectionChanged(PC->GetSelectedCharacterIndex(), CharacterDefinitions.Num());
}

void ULastFPSCharacterSelectWidget::HandleNextClicked()
{
	ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>();
	if (!PC)
	{
		return;
	}

	PC->SetSelectedCharacterIndex(PC->GetSelectedCharacterIndex() + 1);
	OnSelectionChanged(PC->GetSelectedCharacterIndex(), CharacterDefinitions.Num());
}

void ULastFPSCharacterSelectWidget::HandleCardClicked(int32 Index)
{
	ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>();
	if (!PC)
	{
		return;
	}

	PC->SetSelectedCharacterIndex(Index);
	OnSelectionChanged(PC->GetSelectedCharacterIndex(), CharacterDefinitions.Num());
}
