#include "UI/HUD/Quest/LastFPSQuestTrackerCardWidget.h"

#include "Localization/LastFPSLocalization.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "UI/HUD/Quest/LastFPSQuestObjectiveRowWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSQuestTrackerCard, Log, All);

void ULastFPSQuestTrackerCardWidget::UpdateQuest(
	const FLastFPSTrackedQuest& Quest,
	const FLastFPSQuestTrackerViewer& Viewer)
{
	const bool bQuestChanged = DisplayedQuestId != Quest.QuestId;
	DisplayedQuestId = Quest.QuestId;

	if (Text_Title)
	{
		Text_Title->SetText(Quest.Title);
	}

	if (Text_Category)
	{
		Text_Category->SetText(FLastFPSLocalization::GetUIEnumText(
			StaticEnum<ELastFPSQuestType>(), static_cast<int64>(Quest.Type)));
	}

	if (Img_Accent)
	{
		Img_Accent->SetColorAndOpacity(
			Quest.Type == ELastFPSQuestType::Main ? MainAccentColor : SideAccentColor);
	}

	const int32 RowCount = FMath::Min(Quest.Objectives.Num(), MaxObjectiveRows);
	for (int32 RowIndex = 0; RowIndex < RowCount; ++RowIndex)
	{
		ULastFPSQuestObjectiveRowWidget* Row = AcquireObjectiveRow(RowIndex);
		if (!Row)
		{
			break;
		}

		const FLastFPSTrackedObjective& Objective = Quest.Objectives[RowIndex];

		FLastFPSQuestObjectiveRowDisplay Display;
		Display.bHasDistance = Objective.bHasGuidanceLocation && Viewer.bValid;
		if (Display.bHasDistance)
		{
			Display.DistanceMeters = FVector::Dist(Viewer.Location, Objective.GuidanceLocation) / 100.f;
		}

		Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Row->UpdateObjective(Objective, Display);
	}

	// 남는 줄은 파괴하지 않고 접어 둬서 다음 갱신에 그대로 재사용한다.
	for (int32 RowIndex = RowCount; RowIndex < ObjectiveRows.Num(); ++RowIndex)
	{
		if (ObjectiveRows[RowIndex])
		{
			ObjectiveRows[RowIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	OnQuestCardUpdated(Quest.Type, bQuestChanged);
}

ULastFPSQuestObjectiveRowWidget* ULastFPSQuestTrackerCardWidget::AcquireObjectiveRow(const int32 RowIndex)
{
	if (ObjectiveRows.IsValidIndex(RowIndex))
	{
		return ObjectiveRows[RowIndex];
	}

	if (!Box_Objectives)
	{
		return nullptr;
	}

	if (!ObjectiveRowClass)
	{
		if (!bObjectiveRowClassWarningLogged)
		{
			bObjectiveRowClassWarningLogged = true;
			UE_LOG(LogLastFPSQuestTrackerCard, Warning,
				TEXT("[QuestTracker] %s: 목표 줄을 만들지 못했습니다. WBP 의 ObjectiveRowClass 에 WBP_QuestObjectiveRow 가 지정됐는지 확인하세요."),
				*GetName());
		}
		return nullptr;
	}

	ULastFPSQuestObjectiveRowWidget* Row =
		CreateWidget<ULastFPSQuestObjectiveRowWidget>(this, ObjectiveRowClass);
	if (!Row)
	{
		return nullptr;
	}

	Box_Objectives->AddChild(Row);
	ObjectiveRows.Add(Row);
	return Row;
}
