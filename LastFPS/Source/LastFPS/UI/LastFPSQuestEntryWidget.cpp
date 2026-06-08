#include "UI/LastFPSQuestEntryWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void ULastFPSQuestEntryWidget::SetupQuest(const FLastFPSQuestData& InQuest)
{
	if (TB_Title)
	{
		TB_Title->SetText(InQuest.Title);
	}

	if (TB_Summary)
	{
		TB_Summary->SetText(InQuest.Summary);
	}

	if (TB_Status)
	{
		TB_Status->SetText(StatusToText(InQuest.Status));
	}

	if (TB_Reward)
	{
		TB_Reward->SetText(InQuest.RewardText);
	}

	if (Img_Icon)
	{
		if (UTexture2D* Tex = InQuest.Icon.LoadSynchronous())
		{
			Img_Icon->SetBrushFromTexture(Tex);
			Img_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Img_Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	OnQuestDisplayed(InQuest.Type, InQuest.Status);
}

FText ULastFPSQuestEntryWidget::StatusToText(ELastFPSQuestStatus Status)
{
	switch (Status)
	{
	case ELastFPSQuestStatus::InProgress:
		return NSLOCTEXT("LastFPS", "Quest_Status_InProgress", "진행중");
	case ELastFPSQuestStatus::Completed:
		return NSLOCTEXT("LastFPS", "Quest_Status_Completed", "완료");
	case ELastFPSQuestStatus::NotStarted:
	default:
		return NSLOCTEXT("LastFPS", "Quest_Status_NotStarted", "미시작");
	}
}
