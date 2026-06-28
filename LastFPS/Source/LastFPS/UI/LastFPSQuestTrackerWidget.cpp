#include "UI/LastFPSQuestTrackerWidget.h"

#include "UI/LastFPSQuestEntryWidget.h"
#include "Data/Tables/LastFPSQuestData.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"

void ULastFPSQuestTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildTracker();
}

void ULastFPSQuestTrackerWidget::RebuildTracker()
{
	if (!Box_TrackerList)
	{
		return;
	}

	Box_TrackerList->ClearChildren();

	int32 NumRows = 0;

	if (QuestTable && EntryWidgetClass)
	{
		QuestTable->ForeachRow<FLastFPSQuestData>(TEXT("ULastFPSQuestTrackerWidget::RebuildTracker"),
			[this, &NumRows](const FName& /*RowName*/, const FLastFPSQuestData& Row)
			{
				if (Row.Status != ELastFPSQuestStatus::InProgress || NumRows >= MaxTrackedQuests)
				{
					return;
				}

				ULastFPSQuestEntryWidget* Entry = CreateWidget<ULastFPSQuestEntryWidget>(this, EntryWidgetClass);
				if (!Entry)
				{
					return;
				}

				Entry->SetupQuest(Row);
				Box_TrackerList->AddChild(Entry);
				++NumRows;
			});
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(NumRows > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}
