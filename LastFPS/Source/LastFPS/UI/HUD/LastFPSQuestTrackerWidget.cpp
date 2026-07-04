#include "UI/HUD/LastFPSQuestTrackerWidget.h"

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

void ULastFPSQuestTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem(this))
	{
		Subsystem->OnQuestStateChanged.AddDynamic(this, &ULastFPSQuestTrackerWidget::HandleQuestStateChanged);
	}

	RebuildTracker();
}

void ULastFPSQuestTrackerWidget::NativeDestruct()
{
	if (ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem(this))
	{
		Subsystem->OnQuestStateChanged.RemoveDynamic(this, &ULastFPSQuestTrackerWidget::HandleQuestStateChanged);
	}

	Super::NativeDestruct();
}

void ULastFPSQuestTrackerWidget::HandleQuestStateChanged()
{
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

	ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem(this);
	const UDataTable* Table = Subsystem ? Subsystem->GetQuestTable() : nullptr;

	if (Table && EntryWidgetClass)
	{
		Table->ForeachRow<FLastFPSQuestData>(TEXT("ULastFPSQuestTrackerWidget::RebuildTracker"),
			[this, Subsystem, &NumRows](const FName& RowName, const FLastFPSQuestData& Row)
			{
				// 런타임 상태 기준으로 "진행중"만 표시.
				if (NumRows >= MaxTrackedQuests || !Subsystem
					|| Subsystem->GetStatus(RowName) != ELastFPSQuestStatus::InProgress)
				{
					return;
				}

				ULastFPSQuestEntryWidget* Entry = CreateWidget<ULastFPSQuestEntryWidget>(this, EntryWidgetClass);
				if (!Entry)
				{
					return;
				}

				Entry->SetupQuest(Subsystem, RowName, Row);
				Box_TrackerList->AddChild(Entry);
				++NumRows;
			});
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(NumRows > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}
