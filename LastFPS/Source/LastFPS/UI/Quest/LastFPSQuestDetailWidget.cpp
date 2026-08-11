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
	BindStateChanged(InSubsystem);
	BoundQuestId = InQuestId;

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

	RefreshRewardSection(InQuest.Reward, InQuest.RewardText);

	// 진행/버튼처럼 상태에 따라 바뀌는 부분은 통지에서도 같은 코드로 다시 그린다.
	RefreshRuntimeState();
}

void ULastFPSQuestDetailWidget::RefreshRuntimeState()
{
	ULastFPSQuestSubsystem* InSubsystem = OwningSubsystem.Get();
	const FLastFPSQuestData* Def = InSubsystem ? InSubsystem->FindQuest(BoundQuestId) : nullptr;
	if (!Def)
	{
		return;
	}

	const FLastFPSQuestData& InQuest = *Def;
	const FName InQuestId = BoundQuestId;
	const ELastFPSQuestStatus RuntimeStatus = InSubsystem->GetStatus(InQuestId);

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
		TB_Status->SetText(ULastFPSQuestEntryWidget::BuildStatusText(
			InSubsystem,
			InQuestId,
			RuntimeStatus));
	}

	TArray<FText> ProgressLines;

	// 일지는 수락 전 목표를 진행 중인 것처럼 0/N으로 표시하지 않는다.
	if (RuntimeStatus == ELastFPSQuestStatus::InProgress)
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
		// 수락은 QuestGiverNPC 대화 또는 시스템 자동 연계 경로에서만 수행한다.
		Btn_Accept->SetVisibility(ESlateVisibility::Collapsed);
		Btn_Accept->OnClicked().RemoveAll(this);
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
		const bool bClaimable = InSubsystem->IsClaimable(InQuestId);
		Btn_Claim->SetVisibility(bClaimable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_Claim->OnClicked().RemoveAll(this);
		if (bClaimable)
		{
			Btn_Claim->OnClicked().AddUObject(this, &ULastFPSQuestDetailWidget::HandleClaimClicked);
		}
	}

	OnQuestTrackStateChanged(InSubsystem->IsQuestTracked(InQuestId));
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

	// 환급은 구매 전 실제 금액을 알 수 없으므로 RewardText의 정책 설명을 구조화 보상과 함께 표시한다.
	if (TB_Reward)
	{
		const bool bUseFallback = !InFallbackText.IsEmpty()
			&& (InReward.PurchaseRefund.IsEnabled() || (InReward.Credits <= 0 && NumItemSlots == 0));
		TB_Reward->SetText(bUseFallback ? InFallbackText : FText::GetEmpty());
		TB_Reward->SetVisibility(bUseFallback ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
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
		// 요청이 거부될 수도 있으므로(진행중이 아닌 퀘스트 등) 결과는 상태 변경 통지로만 반영한다.
		Subsystem->SetQuestTracked(BoundQuestId, !Subsystem->IsQuestTracked(BoundQuestId));
	}
}

void ULastFPSQuestDetailWidget::HandleQuestStateChanged()
{
	if (BoundQuestId.IsNone())
	{
		return;
	}

	// 수락/취소/추적/보상 수령 결과가 상세 패널에도 즉시 반영되도록 상태 의존 표시를 전부 다시 그린다.
	RefreshRuntimeState();
}

void ULastFPSQuestDetailWidget::BindStateChanged(ULastFPSQuestSubsystem* InSubsystem)
{
	if (OwningSubsystem.Get() == InSubsystem)
	{
		return;
	}

	UnbindStateChanged();
	OwningSubsystem = InSubsystem;

	if (InSubsystem)
	{
		InSubsystem->OnQuestStateChanged.AddUniqueDynamic(this, &ULastFPSQuestDetailWidget::HandleQuestStateChanged);
	}
}

void ULastFPSQuestDetailWidget::UnbindStateChanged()
{
	if (ULastFPSQuestSubsystem* Previous = OwningSubsystem.Get())
	{
		Previous->OnQuestStateChanged.RemoveDynamic(this, &ULastFPSQuestDetailWidget::HandleQuestStateChanged);
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

	BindStateChanged(nullptr); // 구독 해제 + 소유 참조 비움

	Super::NativeDestruct();
}
