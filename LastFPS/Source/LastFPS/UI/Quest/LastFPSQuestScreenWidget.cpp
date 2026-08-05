#include "UI/Quest/LastFPSQuestScreenWidget.h"

#include "UI/Quest/LastFPSQuestEntryWidget.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "Data/Tables/LastFPSQuestData.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	ULastFPSQuestSubsystem* GetQuestSubsystem(const UWidget* Widget)
	{
		if (const UWorld* World = Widget ? Widget->GetWorld() : nullptr)
		{
			if (UGameInstance* GI = World->GetGameInstance())
			{
				return GI->GetSubsystem<ULastFPSQuestSubsystem>();
			}
		}
		return nullptr;
	}
}

void ULastFPSQuestScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem(this))
	{
		Subsystem->OnQuestStateChanged.AddUniqueDynamic(this, &ULastFPSQuestScreenWidget::HandleQuestStateChanged);
	}

	RebuildQuestList();
}

void ULastFPSQuestScreenWidget::NativeDestruct()
{
	if (ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem(this))
	{
		Subsystem->OnQuestStateChanged.RemoveDynamic(this, &ULastFPSQuestScreenWidget::HandleQuestStateChanged);
	}

	Super::NativeDestruct();
}

void ULastFPSQuestScreenWidget::HandleQuestStateChanged()
{
	RebuildQuestList();
}

void ULastFPSQuestScreenWidget::RebuildQuestList()
{
	if (!Box_QuestList)
	{
		return;
	}

	Box_QuestList->ClearChildren();

	int32 NumRows = 0;

	ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem(this);
	const UDataTable* Table = Subsystem ? Subsystem->GetQuestTable() : nullptr;

	if (Table && EntryWidgetClass)
	{
		Table->ForeachRow<FLastFPSQuestData>(TEXT("ULastFPSQuestScreenWidget::RebuildQuestList"),
			[this, Subsystem, &NumRows](const FName& RowName, const FLastFPSQuestData& Row)
			{
				if (Row.Category.MatchesAny(ExcludedCategories))
				{
					return;
				}

				ULastFPSQuestEntryWidget* Entry = CreateWidget<ULastFPSQuestEntryWidget>(this, EntryWidgetClass);
				if (!Entry)
				{
					return;
				}

				Entry->SetupQuest(Subsystem, RowName, Row);
				Box_QuestList->AddChild(Entry);
				++NumRows;
			});
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(NumRows > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}
