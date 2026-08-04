#include "UI/HUD/Quest/LastFPSQuestObjectiveRowWidget.h"

#include "Localization/LastFPSLocalization.h"
#include "Quest/LastFPSQuestSubsystem.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void ULastFPSQuestObjectiveRowWidget::UpdateObjective(
	const FLastFPSTrackedObjective& Objective,
	const FLastFPSQuestObjectiveRowDisplay& Display)
{
	// 요구량 1인 목표는 "1 / 1" 이 정보를 주지 않으므로 카운트/게이지를 접는다.
	const bool bShowCount = Objective.RequiredCount > 1;
	const float ProgressRatio = Objective.RequiredCount > 0
		? FMath::Clamp(static_cast<float>(Objective.Progress) / static_cast<float>(Objective.RequiredCount), 0.f, 1.f)
		: 0.f;
	const FLinearColor LabelColor = Objective.bCompleted ? CompletedColor : ActiveColor;

	if (Text_Label)
	{
		Text_Label->SetText(Objective.Label);
		Text_Label->SetColorAndOpacity(FSlateColor(LabelColor));
		Text_Label->SetVisibility(Objective.Label.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible);
	}

	if (Text_Progress)
	{
		if (bShowCount)
		{
			Text_Progress->SetText(FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::RatioFormat),
				FText::AsNumber(Objective.Progress),
				FText::AsNumber(Objective.RequiredCount)));
			Text_Progress->SetColorAndOpacity(FSlateColor(LabelColor));
		}
		Text_Progress->SetVisibility(bShowCount
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (Bar_Progress)
	{
		if (bShowCount)
		{
			Bar_Progress->SetPercent(ProgressRatio);
			Bar_Progress->SetFillColorAndOpacity(ProgressFillColor);
		}
		Bar_Progress->SetVisibility(bShowCount
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (Text_Distance)
	{
		if (Display.bHasDistance)
		{
			Text_Distance->SetText(FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::DistanceMetersFormat),
				FText::AsNumber(FMath::RoundToInt(Display.DistanceMeters))));
		}
		Text_Distance->SetVisibility(Display.bHasDistance
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (Icon_Pending)
	{
		Icon_Pending->SetVisibility(Objective.bCompleted
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	if (Icon_Complete)
	{
		Icon_Complete->SetVisibility(Objective.bCompleted
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	OnObjectiveRowUpdated(Objective.bCompleted, ProgressRatio);
}
