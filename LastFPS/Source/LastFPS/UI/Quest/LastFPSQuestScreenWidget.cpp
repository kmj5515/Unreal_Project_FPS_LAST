#include "UI/Quest/LastFPSQuestScreenWidget.h"

#include "UI/Quest/LastFPSQuestEntryWidget.h"
#include "UI/Quest/LastFPSQuestGroupWidget.h"
#include "UI/Quest/LastFPSQuestDetailWidget.h"
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

void ULastFPSQuestScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (DetailWidget)
	{
		// 선택 전에는 내용을 숨기되 레이아웃 공간은 유지한다.
		// Collapsed 로 두면 목록 영역이 늘어났다가 선택 시 다시 줄어드는 흔들림이 생긴다.
		DetailWidget->SetVisibility(ESlateVisibility::Hidden);
	}
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
	if (Box_QuestList) Box_QuestList->ClearChildren();
	if (Box_MainQuests) Box_MainQuests->ClearChildren();
	if (Box_SideQuests) Box_SideQuests->ClearChildren();

	int32 NumRows = 0;

	ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem(this);
	const UDataTable* Table = Subsystem ? Subsystem->GetQuestTable() : nullptr;

	if (Table && EntryWidgetClass)
	{
		TMap<FString, class ULastFPSQuestGroupWidget*> GroupWidgetMap;

		Table->ForeachRow<FLastFPSQuestData>(TEXT("ULastFPSQuestScreenWidget::RebuildQuestList"),
			[this, Subsystem, &NumRows, &GroupWidgetMap](const FName& RowName, const FLastFPSQuestData& Row)
			{
				if (Subsystem && Subsystem->IsQuestMappedToAnyMap(RowName))
				{
					return;
				}

				ULastFPSQuestEntryWidget* Entry = CreateWidget<ULastFPSQuestEntryWidget>(this, EntryWidgetClass);
				if (!Entry)
				{
					return;
				}

				Entry->SetupQuest(Subsystem, RowName, Row);
				Entry->OnQuestSelected.AddDynamic(this, &ULastFPSQuestScreenWidget::HandleQuestSelected);

				FString GroupTitleStr = Row.QuestGroupTitle.ToString();
				if (GroupTitleStr.IsEmpty())
				{
					GroupTitleStr = TEXT("Uncategorized");
				}

				UPanelWidget* TargetBox = Box_MainQuests ? Box_MainQuests : Box_QuestList;
				
				if (GroupWidgetClass)
				{
					ULastFPSQuestGroupWidget* GroupWidget = nullptr;
					if (ULastFPSQuestGroupWidget** FoundGroup = GroupWidgetMap.Find(GroupTitleStr))
					{
						GroupWidget = *FoundGroup;
					}
					else
					{
						GroupWidget = CreateWidget<ULastFPSQuestGroupWidget>(this, GroupWidgetClass);
						if (GroupWidget)
						{
							GroupWidget->SetupGroup(FText::FromString(GroupTitleStr), Row.QuestGroupSubtitle);
							GroupWidgetMap.Add(GroupTitleStr, GroupWidget);
							if (TargetBox)
							{
								TargetBox->AddChild(GroupWidget);
							}
						}
					}

					if (GroupWidget)
					{
						GroupWidget->AddQuestEntry(Entry);
					}
				}
				else if (TargetBox)
				{
					// 그룹 위젯 클래스가 없으면 예전처럼 직접 추가
					TargetBox->AddChild(Entry);
				}

				++NumRows;
			});
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(NumRows > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void ULastFPSQuestScreenWidget::HandleQuestSelected(FName QuestId)
{
	ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem(this);
	if (DetailWidget && Subsystem)
	{
		if (const FLastFPSQuestData* Row = Subsystem->FindQuest(QuestId))
		{
			DetailWidget->SetVisibility(ESlateVisibility::Visible);
			DetailWidget->SetupQuest(Subsystem, QuestId, *Row);
		}
	}
}
