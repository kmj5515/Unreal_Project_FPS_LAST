#include "UI/Quest/LastFPSQuestScreenWidget.h"

#include "UI/Quest/LastFPSQuestEntryWidget.h"
#include "Data/Tables/LastFPSQuestData.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"

void ULastFPSQuestScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

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

	if (QuestTable && EntryWidgetClass)
	{
		// DataTable 행 순서대로 엔트리 생성. (정렬/필터는 추후 서브시스템에서)
		QuestTable->ForeachRow<FLastFPSQuestData>(TEXT("ULastFPSQuestScreenWidget::RebuildQuestList"),
			[this, &NumRows](const FName& /*RowName*/, const FLastFPSQuestData& Row)
			{
				ULastFPSQuestEntryWidget* Entry = CreateWidget<ULastFPSQuestEntryWidget>(this, EntryWidgetClass);
				if (!Entry)
				{
					return;
				}

				Entry->SetupQuest(Row);
				Box_QuestList->AddChild(Entry);
				++NumRows;
			});
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(NumRows > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}
