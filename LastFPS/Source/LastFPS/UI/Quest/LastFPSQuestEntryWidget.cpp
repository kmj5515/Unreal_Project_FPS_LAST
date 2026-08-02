#include "UI/Quest/LastFPSQuestEntryWidget.h"

#include "Localization/LastFPSLocalization.h"
#include "Quest/LastFPSQuestSubsystem.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"

void ULastFPSQuestEntryWidget::SetupQuest(ULastFPSQuestSubsystem* InSubsystem, FName InQuestId, const FLastFPSQuestData& InQuest)
{
	OwningSubsystem = InSubsystem;
	BoundQuestId = InQuestId;

	const ELastFPSQuestStatus RuntimeStatus = InSubsystem ? InSubsystem->GetStatus(InQuestId) : InQuest.Status;

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
		TB_Status->SetText(StatusToText(RuntimeStatus));
	}

	if (TB_Reward)
	{
		TB_Reward->SetText(InQuest.RewardText);
	}

	// 진행도 — 데이터의 목표 문구와 실제 요구량을 줄별로 표시한다.
	// ClearEncounter 요구량은 QuestTable의 중복 숫자가 아니라 현재 Mode의 Encounter Profile이 제공한다.
	if (TB_Progress)
	{
		if (InQuest.Objectives.Num() > 0)
		{
			TArray<FText> ProgressLines;
			for (int32 i = 0; i < InQuest.Objectives.Num(); ++i)
			{
				const int32 Cur = InSubsystem ? InSubsystem->GetObjectiveProgress(InQuestId, i) : 0;
				const int32 Req = InSubsystem
					? InSubsystem->GetObjectiveRequiredCount(InQuestId, i)
					: InQuest.Objectives[i].RequiredCount;

				if (InQuest.bSequentialObjectives && Cur >= Req)
				{
					continue;
				}

				const FText ProgressValue = FText::Format(
					FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::RatioFormat),
					FText::AsNumber(Cur),
					FText::AsNumber(Req));
				ProgressLines.Add(InQuest.Objectives[i].Label.IsEmpty()
					? ProgressValue
					: FText::Format(
						FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::ObjectiveProgressLineFormat),
						InQuest.Objectives[i].Label,
						ProgressValue));

				if (InQuest.bSequentialObjectives)
				{
					break;
				}
			}
			TB_Progress->SetText(FText::Join(FText::FromString(TEXT("\n")), ProgressLines));
			TB_Progress->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			TB_Progress->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 보상 수령 버튼 — Completed 일 때만 노출/활성. (Claimed/진행중엔 숨김)
	if (Btn_Claim)
	{
		const bool bClaimable = InSubsystem && InSubsystem->IsClaimable(InQuestId);
		Btn_Claim->SetVisibility(bClaimable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_Claim->OnClicked.RemoveDynamic(this, &ULastFPSQuestEntryWidget::HandleClaimClicked);
		if (bClaimable)
		{
			Btn_Claim->OnClicked.AddUniqueDynamic(this, &ULastFPSQuestEntryWidget::HandleClaimClicked);
		}
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

	OnQuestDisplayed(InQuest.Type, RuntimeStatus);
}

void ULastFPSQuestEntryWidget::HandleClaimClicked()
{
	if (ULastFPSQuestSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->TryClaimReward(BoundQuestId);
		// 지급 성공 시 OnQuestStateChanged → 부모가 목록을 재구성해 버튼이 사라진다.
	}
}

void ULastFPSQuestEntryWidget::NativeDestruct()
{
	if (Btn_Claim)
	{
		Btn_Claim->OnClicked.RemoveDynamic(this, &ULastFPSQuestEntryWidget::HandleClaimClicked);
	}
	Super::NativeDestruct();
}

FText ULastFPSQuestEntryWidget::StatusToText(ELastFPSQuestStatus Status)
{
	switch (Status)
	{
	case ELastFPSQuestStatus::InProgress:
		return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestStatusInProgress);
	case ELastFPSQuestStatus::Completed:
		return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestStatusCompleted);
	case ELastFPSQuestStatus::Claimed:
		return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestStatusClaimed);
	case ELastFPSQuestStatus::Locked:
		return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestStatusLocked);
	case ELastFPSQuestStatus::NotStarted:
	default:
		return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestStatusNotStarted);
	}
}
