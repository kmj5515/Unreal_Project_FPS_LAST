#include "UI/Quest/LastFPSQuestGroupWidget.h"
#include "UI/Quest/LastFPSQuestEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/ExpandableArea.h"

void ULastFPSQuestGroupWidget::SetupGroup(const FText& InGroupTitle, const FText& InGroupSubtitle)
{
	if (TB_GroupTitle)
	{
		TB_GroupTitle->SetText(InGroupTitle);
	}

	if (TB_GroupSubtitle)
	{
		TB_GroupSubtitle->SetText(InGroupSubtitle);
	}
}

void ULastFPSQuestGroupWidget::AddQuestEntry(ULastFPSQuestEntryWidget* EntryWidget)
{
	if (Box_Entries && EntryWidget)
	{
		Box_Entries->AddChildToVerticalBox(EntryWidget);
	}
}
