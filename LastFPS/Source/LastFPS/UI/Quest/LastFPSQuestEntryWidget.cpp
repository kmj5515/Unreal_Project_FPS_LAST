#include "UI/Quest/LastFPSQuestEntryWidget.h"

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

	// 진행도 — 목표별 "cur/req" 를 이어붙인다. 목표가 없으면 숨김.
	if (TB_Progress)
	{
		if (InQuest.Objectives.Num() > 0)
		{
			FString Progress;
			for (int32 i = 0; i < InQuest.Objectives.Num(); ++i)
			{
				const int32 Cur = InSubsystem ? InSubsystem->GetObjectiveProgress(InQuestId, i) : 0;
				const int32 Req = InQuest.Objectives[i].RequiredCount;
				if (i > 0)
				{
					Progress += TEXT("  ·  ");
				}
				Progress += FString::Printf(TEXT("%d/%d"), Cur, Req);
			}
			TB_Progress->SetText(FText::FromString(Progress));
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
			Btn_Claim->OnClicked.AddDynamic(this, &ULastFPSQuestEntryWidget::HandleClaimClicked);
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
		return NSLOCTEXT("LastFPS", "Quest_Status_InProgress", "진행중");
	case ELastFPSQuestStatus::Completed:
		return NSLOCTEXT("LastFPS", "Quest_Status_Completed", "완료");
	case ELastFPSQuestStatus::Claimed:
		return NSLOCTEXT("LastFPS", "Quest_Status_Claimed", "수령완료");
	case ELastFPSQuestStatus::NotStarted:
	default:
		return NSLOCTEXT("LastFPS", "Quest_Status_NotStarted", "미시작");
	}
}
