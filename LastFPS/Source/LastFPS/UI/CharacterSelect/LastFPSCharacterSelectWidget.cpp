#include "UI/CharacterSelect/LastFPSCharacterSelectWidget.h"

#include "UI/CharacterSelect/LastFPSCharacterCardWidget.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Definitions/LastFPSHeroDefinition.h"
#include "Data/Definitions/LastFPSCharacterRoster.h"
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

	const ULastFPSCharacterRoster* Roster = GetCharacterRoster();

	TObjectPtr<ULastFPSCharacterCardWidget> Cards[] = { Card_0, Card_1, Card_2 };
	for (int32 i = 0; i < 3; ++i)
	{
		if (Cards[i])
		{
			Cards[i]->CardIndex = i;
			Cards[i]->OnCardClicked.BindUObject(this, &ULastFPSCharacterSelectWidget::HandleCardClicked);

			const ULastFPSCharacterDefinition* Def = Roster ? Roster->GetDefinition(i) : nullptr;
			Cards[i]->SetupCard(Def);
		}
	}

	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		const int32 SelectedIndex = PC->GetSelectedCharacterIndex();
		UpdateCardSelection(SelectedIndex);
		OnSelectionChanged(SelectedIndex, Roster ? Roster->Num() : 0);
	}
}

const ULastFPSCharacterRoster* ULastFPSCharacterSelectWidget::GetCharacterRoster() const
{
	if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
	{
		return GI->GetCharacterRoster();
	}
	return nullptr;
}

void ULastFPSCharacterSelectWidget::UpdateCardSelection(int32 SelectedIndex)
{
	TObjectPtr<ULastFPSCharacterCardWidget> Cards[] = { Card_0, Card_1, Card_2 };
	for (int32 i = 0; i < 3; ++i)
	{
		if (Cards[i])
		{
			Cards[i]->SetSelected(i == SelectedIndex);
		}
	}
}

void ULastFPSCharacterSelectWidget::OnSelectionChanged_Implementation(int32 NewIndex, int32 TotalCount)
{
	const ULastFPSCharacterRoster* Roster = GetCharacterRoster();
	const ULastFPSCharacterDefinition* Def = Roster ? Roster->GetDefinition(NewIndex) : nullptr;

	// Role/Description 은 히어로 전용 데이터이므로 Hero 로 캐스팅해서 읽는다.
	const ULastFPSHeroDefinition* HeroDef = Cast<ULastFPSHeroDefinition>(Def);

	if (TB_CharName)
	{
		TB_CharName->SetText(Def ? Def->DisplayName : FText::GetEmpty());
	}
	if (TB_CharRole)
	{
		TB_CharRole->SetText(HeroDef ? HeroDef->Role : FText::GetEmpty());
	}
	if (TB_CharDesc)
	{
		TB_CharDesc->SetText(HeroDef ? HeroDef->Description : FText::GetEmpty());
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

void ULastFPSCharacterSelectWidget::HandleCardClicked(int32 Index)
{
	ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>();
	if (!PC)
	{
		return;
	}

	PC->SetSelectedCharacterIndex(Index);

	// PC가 clamp한 최종 인덱스를 기준으로 갱신 (선택 표시는 BP 오버라이드와 무관하게 항상 반영)
	const int32 SelectedIndex = PC->GetSelectedCharacterIndex();
	const ULastFPSCharacterRoster* Roster = GetCharacterRoster();
	UpdateCardSelection(SelectedIndex);
	OnSelectionChanged(SelectedIndex, Roster ? Roster->Num() : 0);
}
