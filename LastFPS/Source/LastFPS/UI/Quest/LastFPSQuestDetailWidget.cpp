#include "UI/Quest/LastFPSQuestDetailWidget.h"
#include "UI/Quest/LastFPSQuestEntryWidget.h"
#include "UI/Inventory/LastFPSItemSlotWidget.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Engine/DataTable.h"
#include "Localization/LastFPSLocalization.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/WrapBox.h"
#include "Engine/GameInstance.h"
#include "UI/Framework/LastFPSButtonBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSQuestDetail, Log, All);

void ULastFPSQuestDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Accept)
	{
		Btn_Accept->SetButtonText(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestActionAccept));
	}
	if (Btn_Cancel)
	{
		Btn_Cancel->SetButtonText(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestActionCancel));
	}
	if (Btn_Track)
	{
		Btn_Track->SetButtonText(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestActionTrack));
	}
	if (Btn_Claim)
	{
		Btn_Claim->SetButtonText(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestActionClaim));
	}

	if (TB_Progress_txt)
	{
		TB_Progress_txt->SetText(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestHeaderProgress));
	}
	if (TB_LabelReward)
	{
		TB_LabelReward->SetText(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestHeaderReward));
	}
}

void ULastFPSQuestDetailWidget::SetupQuest(ULastFPSQuestSubsystem* InSubsystem, FName InQuestId, const FLastFPSQuestData& InQuest)
{
	OwningSubsystem = InSubsystem;
	BoundQuestId = InQuestId;

	const ELastFPSQuestStatus RuntimeStatus = InSubsystem ? InSubsystem->GetStatus(InQuestId) : InQuest.Status;

	if (TB_Title)
	{
		TB_Title->SetText(InQuest.Title);
	}

	if (TB_RecommendedLevel)
	{
		TB_RecommendedLevel->SetText(FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestRecommendedLevelFormat),
			InQuest.RecommendedLevel));
	}

	if (Img_Banner)
	{
		if (UTexture2D* BannerTex = InQuest.BannerImage.LoadSynchronous())
		{
			Img_Banner->SetBrushFromTexture(BannerTex);
			Img_Banner->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Img_Banner->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (TB_Description)
	{
		TB_Description->SetText(InQuest.Description);
	}

	// 아래 목표 루프는 "첫 미완료 목표"를 찾을 때 빈 텍스트인지로 판별한다.
	// 이전 선택의 텍스트가 남아 있으면 판별이 실패하므로 루프 전에 반드시 비운다.
	if (TB_CurrentObjective)
	{
		TB_CurrentObjective->SetText(FText::GetEmpty());
	}
	if (Box_ObjectiveMarker)
	{
		Box_ObjectiveMarker->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TB_Status)
	{
		TB_Status->SetText(ULastFPSQuestEntryWidget::StatusToText(RuntimeStatus));
	}

	RefreshRewardSection(InQuest.Reward, InQuest.RewardText);

	// 완료/수령 상태에서는 남은 목표가 없으므로 진행 상황 자체를 표시하지 않는다.
	const bool bQuestFinished =
		RuntimeStatus == ELastFPSQuestStatus::Completed || RuntimeStatus == ELastFPSQuestStatus::Claimed;

	TArray<FText> ProgressLines;

	if (!bQuestFinished)
	{
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
			FText LineText = InQuest.Objectives[i].Label.IsEmpty()
				? ProgressValue
				: FText::Format(
					FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::ObjectiveProgressLineFormat),
					InQuest.Objectives[i].Label,
					ProgressValue);

			ProgressLines.Add(LineText);

			// 첫 번째 활성 목표를 Current Objective 로 표시
			if (TB_CurrentObjective && Cur < Req && TB_CurrentObjective->GetText().IsEmpty())
			{
				TB_CurrentObjective->SetText(InQuest.Objectives[i].Label);
			}

			if (InQuest.bSequentialObjectives)
			{
				break;
			}
		}
	}

	// 표시할 목표 줄이 하나도 없으면 헤더 라벨까지 같이 숨겨 빈 제목만 남지 않게 한다.
	const bool bShowProgress = ProgressLines.Num() > 0;
	const bool bShowCurrentObjective =
		TB_CurrentObjective && !TB_CurrentObjective->GetText().IsEmpty();

	if (TB_CurrentObjective)
	{
		TB_CurrentObjective->SetVisibility(
			bShowCurrentObjective ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Box_ObjectiveMarker)
	{
		Box_ObjectiveMarker->SetVisibility(
			bShowCurrentObjective ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (TB_Progress)
	{
		TB_Progress->SetText(bShowProgress
			? FText::Join(FText::FromString(TEXT("\n")), ProgressLines)
			: FText::GetEmpty());
		TB_Progress->SetVisibility(bShowProgress ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (TB_Progress_txt)
	{
		TB_Progress_txt->SetVisibility(
			bShowProgress ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Btn_Accept)
	{
		Btn_Accept->SetVisibility(RuntimeStatus == ELastFPSQuestStatus::NotStarted ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_Accept->OnClicked().RemoveAll(this);
		if (RuntimeStatus == ELastFPSQuestStatus::NotStarted)
		{
			Btn_Accept->OnClicked().AddUObject(this, &ULastFPSQuestDetailWidget::HandleAcceptClicked);
		}
	}

	if (Btn_Cancel)
	{
		Btn_Cancel->SetVisibility(RuntimeStatus == ELastFPSQuestStatus::InProgress ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_Cancel->OnClicked().RemoveAll(this);
		if (RuntimeStatus == ELastFPSQuestStatus::InProgress)
		{
			Btn_Cancel->OnClicked().AddUObject(this, &ULastFPSQuestDetailWidget::HandleCancelClicked);
		}
	}

	if (Btn_Track)
	{
		Btn_Track->SetVisibility(RuntimeStatus == ELastFPSQuestStatus::InProgress ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_Track->OnClicked().RemoveAll(this);
		if (RuntimeStatus == ELastFPSQuestStatus::InProgress)
		{
			Btn_Track->OnClicked().AddUObject(this, &ULastFPSQuestDetailWidget::HandleTrackClicked);
		}
	}

	if (Btn_Claim)
	{
		const bool bClaimable = InSubsystem && InSubsystem->IsClaimable(InQuestId);
		Btn_Claim->SetVisibility(bClaimable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_Claim->OnClicked().RemoveAll(this);
		if (bClaimable)
		{
			Btn_Claim->OnClicked().AddUObject(this, &ULastFPSQuestDetailWidget::HandleClaimClicked);
		}
	}

	if (InSubsystem)
	{
		OnQuestTrackStateChanged(InSubsystem->IsQuestTracked(InQuestId));
	}
}

void ULastFPSQuestDetailWidget::RefreshRewardSection(const FLastFPSQuestReward& InReward, const FText& InFallbackText)
{
	if (TB_RewardCredits)
	{
		const bool bHasCredits = InReward.Credits > 0;
		if (bHasCredits)
		{
			TB_RewardCredits->SetText(FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestRewardCreditsFormat),
				FText::AsNumber(InReward.Credits)));
		}
		TB_RewardCredits->SetVisibility(bHasCredits ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	int32 NumItemSlots = 0;

	if (WrapBox_RewardItems)
	{
		WrapBox_RewardItems->ClearChildren();

		// 아이템 정의는 GameData 레지스트리의 안정적인 태그 계약으로만 조회한다.
		// (EquipmentSubsystem 의 행 조회는 내부 구현이라 화면에서 의존하지 않는다.)
		UGameInstance* GameInstance = GetGameInstance();
		ULastFPSGameDataSubsystem* GameData =
			GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
		const UDataTable* ItemTable =
			GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item) : nullptr;

		if (ItemTable && RewardSlotWidgetClass)
		{
			static const TCHAR* RewardLookupContext = TEXT("ULastFPSQuestDetailWidget::RefreshRewardSection");

			for (const FLastFPSItemGrant& Grant : InReward.Items)
			{
				const FLastFPSItemData* ItemDef =
					ItemTable->FindRow<FLastFPSItemData>(Grant.RowId, RewardLookupContext, /*bWarnIfRowMissing=*/false);
				if (!ItemDef)
				{
					// 보상 행이 아이템 테이블과 어긋난 상태. 조용히 넘기면 데이터 오류를 못 찾는다.
					UE_LOG(LogLastFPSQuestDetail, Warning,
						TEXT("QuestDetail: 보상 아이템 '%s' 를 아이템 테이블에서 찾지 못해 표시를 건너뜁니다. (Quest=%s)"),
						*Grant.RowId.ToString(), *BoundQuestId.ToString());
					continue;
				}

				ULastFPSItemSlotWidget* RewardSlot = CreateWidget<ULastFPSItemSlotWidget>(this, RewardSlotWidgetClass);
				if (!RewardSlot)
				{
					continue;
				}

				RewardSlot->SetupSlot(*ItemDef, Grant.RowId, Grant.Count);
				WrapBox_RewardItems->AddChildToWrapBox(RewardSlot);
				++NumItemSlots;
			}
		}

		WrapBox_RewardItems->SetVisibility(NumItemSlots > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 구조화 보상이 하나도 없을 때만 자유 표기 문자열로 대체한다.
	if (TB_Reward)
	{
		const bool bUseFallback = InReward.Credits <= 0 && NumItemSlots == 0 && !InFallbackText.IsEmpty();
		TB_Reward->SetText(bUseFallback ? InFallbackText : FText::GetEmpty());
		TB_Reward->SetVisibility(bUseFallback ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void ULastFPSQuestDetailWidget::HandleAcceptClicked()
{
	if (ULastFPSQuestSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->AcceptQuest(BoundQuestId);
	}
}

void ULastFPSQuestDetailWidget::HandleCancelClicked()
{
	if (ULastFPSQuestSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->CancelQuest(BoundQuestId);
	}
}

void ULastFPSQuestDetailWidget::HandleTrackClicked()
{
	if (ULastFPSQuestSubsystem* Subsystem = OwningSubsystem.Get())
	{
		bool bCurrentlyTracked = Subsystem->IsQuestTracked(BoundQuestId);
		Subsystem->SetQuestTracked(BoundQuestId, !bCurrentlyTracked);
		OnQuestTrackStateChanged(!bCurrentlyTracked);
	}
}

void ULastFPSQuestDetailWidget::HandleClaimClicked()
{
	if (ULastFPSQuestSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->TryClaimReward(BoundQuestId);
	}
}

void ULastFPSQuestDetailWidget::NativeDestruct()
{
	if (Btn_Accept) Btn_Accept->OnClicked().RemoveAll(this);
	if (Btn_Cancel) Btn_Cancel->OnClicked().RemoveAll(this);
	if (Btn_Track) Btn_Track->OnClicked().RemoveAll(this);
	if (Btn_Claim) Btn_Claim->OnClicked().RemoveAll(this);

	Super::NativeDestruct();
}
