#include "UI/HUD/Quest/LastFPSQuestTrackerCardWidget.h"

#include "Localization/LastFPSLocalization.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "UI/HUD/Quest/LastFPSQuestObjectiveRowWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSQuestTrackerCard, Log, All);

void ULastFPSQuestTrackerCardWidget::UpdateQuest(
	const FLastFPSTrackedQuest& Quest,
	const FLastFPSQuestTrackerViewer& Viewer)
{
	const bool bQuestChanged = DisplayedQuestId != Quest.QuestId;
	int32 ActiveObjectiveIndex = Quest.Objectives.IndexOfByPredicate(
		[](const FLastFPSTrackedObjective& Objective)
		{
			return !Objective.bCompleted;
		});
	if (ActiveObjectiveIndex == INDEX_NONE && !Quest.Objectives.IsEmpty())
	{
		// 완료 전이가 HUD에 전달되는 짧은 구간에도 마지막으로 갱신된 문구를 유지한다.
		ActiveObjectiveIndex = Quest.Objectives.Num() - 1;
	}

	const bool bObjectiveChanged =
		bQuestChanged || DisplayedObjectiveIndex != ActiveObjectiveIndex;
	DisplayedQuestId = Quest.QuestId;
	DisplayedObjectiveIndex = ActiveObjectiveIndex;

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

	const int32 RowCount = ActiveObjectiveIndex != INDEX_NONE ? 1 : 0;
	if (RowCount > 0)
	{
		ULastFPSQuestObjectiveRowWidget* Row = AcquireObjectiveRow(0);
		if (Row)
		{
			const FLastFPSTrackedObjective& Objective = Quest.Objectives[ActiveObjectiveIndex];

			FLastFPSQuestObjectiveRowDisplay Display;
			Display.bHasDistance = Objective.bHasGuidanceLocation && Viewer.bValid;
			if (Display.bHasDistance)
			{
				Display.DistanceMeters = FVector::Dist(Viewer.Location, Objective.GuidanceLocation) / 100.f;
			}

			Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Row->UpdateObjective(Objective, Display);
		}
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
	if (bObjectiveChanged)
	{
		StartUpdateAnimation();
	}
}

void ULastFPSQuestTrackerCardWidget::NativeDestruct()
{
	FinishUpdateAnimation();
	Super::NativeDestruct();
}

void ULastFPSQuestTrackerCardWidget::StartUpdateAnimation()
{
	UWorld* World = GetWorld();
	if (!bPlayUpdateAnimation || UpdateAnimationDuration <= 0.f || !World)
	{
		FinishUpdateAnimation();
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.ClearTimer(UpdateAnimationTimerHandle);

	UpdateAnimationStartTime = World->GetTimeSeconds();
	UWidget* AnimationTarget = GetUpdateAnimationTarget();
	AnimationTarget->SetRenderOpacity(0.f);
	AnimationTarget->SetRenderTranslation(UpdateAnimationOffset);
	TimerManager.SetTimer(
		UpdateAnimationTimerHandle,
		this,
		&ULastFPSQuestTrackerCardWidget::UpdateAnimationFrame,
		1.f / 60.f,
		true);
}

void ULastFPSQuestTrackerCardWidget::UpdateAnimationFrame()
{
	UWorld* World = GetWorld();
	if (!World || UpdateAnimationDuration <= 0.f)
	{
		FinishUpdateAnimation();
		return;
	}

	const float Alpha = FMath::Clamp(
		static_cast<float>((World->GetTimeSeconds() - UpdateAnimationStartTime) / UpdateAnimationDuration),
		0.f,
		1.f);
	const float EasedAlpha = 1.f - FMath::Pow(1.f - Alpha, 3.f);
	UWidget* AnimationTarget = GetUpdateAnimationTarget();
	AnimationTarget->SetRenderOpacity(EasedAlpha);
	AnimationTarget->SetRenderTranslation(UpdateAnimationOffset * (1.f - EasedAlpha));

	if (Alpha >= 1.f)
	{
		FinishUpdateAnimation();
	}
}

void ULastFPSQuestTrackerCardWidget::FinishUpdateAnimation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateAnimationTimerHandle);
	}

	UWidget* AnimationTarget = GetUpdateAnimationTarget();
	AnimationTarget->SetRenderOpacity(1.f);
	AnimationTarget->SetRenderTranslation(FVector2D::ZeroVector);
}

UWidget* ULastFPSQuestTrackerCardWidget::GetUpdateAnimationTarget()
{
	if (ObjectiveRows.IsValidIndex(0) && ObjectiveRows[0])
	{
		return ObjectiveRows[0];
	}

	return this;
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
