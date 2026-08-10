#include "UI/Result/LastFPSMissionResultWidget.h"

#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Localization/LastFPSLocalization.h"
#include "UI/Equipment/LastFPSStatEntryWidget.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Framework/LastFPSPopupSubsystem.h"
#include "UI/Framework/LastFPSPopupTags.h"
#include "UI/Inventory/LastFPSItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSMissionResult, Log, All);

namespace
{
	/** 통계 행은 증감이 아니라 실적이므로 부호 색상을 쓰지 않는다. */
	constexpr int32 NeutralStatSign = 0;
}

ULastFPSMissionResultWidget* ULastFPSMissionResultWidget::ShowPopup(
	const UObject* WorldContext,
	const FLastFPSMissionResult& InResult)
{
	ULastFPSMissionResultWidget* Widget =
		ULastFPSPopupSubsystem::ShowPopup<ULastFPSMissionResultWidget>(
			WorldContext, LastFPSPopupTags::MissionResult());
	if (Widget)
	{
		Widget->SetupResult(InResult);
	}

	return Widget;
}

void ULastFPSMissionResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked().AddUObject(
			this, &ULastFPSMissionResultWidget::HandleConfirmClicked);
	}
}

bool ULastFPSMissionResultWidget::NativeOnHandleBackAction()
{
	HandleConfirmClicked();
	return true;
}

void ULastFPSMissionResultWidget::HandleConfirmClicked()
{
	CompleteDialog(ECommonMessagingResult::Confirmed);
}

void ULastFPSMissionResultWidget::SetupResult(const FLastFPSMissionResult& InResult)
{
	RefreshHeader(InResult);
	RefreshScore(InResult);
	RefreshCombatStats(InResult.CombatStats);
	RefreshItems(InResult.Items);
}

void ULastFPSMissionResultWidget::RefreshHeader(const FLastFPSMissionResult& InResult)
{
	if (TB_MissionName)
	{
		TB_MissionName->SetText(InResult.MissionName);
	}

	if (TB_ElapsedTime)
	{
		// 측정되지 않은 임무(음수)는 0초로 표기하면 오해를 부르므로 아예 감춘다.
		const bool bHasElapsed = InResult.ElapsedSeconds >= 0.f;
		if (bHasElapsed)
		{
			TB_ElapsedTime->SetText(
				FText::AsTimespan(FTimespan::FromSeconds(InResult.ElapsedSeconds)));
		}
		TB_ElapsedTime->SetVisibility(
			bHasElapsed ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (TB_Credits)
	{
		const bool bHasCredits = InResult.Credits > 0;
		if (bHasCredits)
		{
			TB_Credits->SetText(FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestRewardCreditsFormat),
				FText::AsNumber(InResult.Credits)));
		}
		TB_Credits->SetVisibility(
			bHasCredits ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Img_Portrait)
	{
		// 결과 화면은 임무당 한 번만 열리므로 동기 로드로 한 프레임을 잡아도 체감 비용이 없다.
		UTexture2D* Portrait = InResult.CharacterPortrait.LoadSynchronous();
		if (Portrait)
		{
			Img_Portrait->SetBrushFromTexture(Portrait);
		}
		Img_Portrait->SetVisibility(
			Portrait ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void ULastFPSMissionResultWidget::RefreshScore(const FLastFPSMissionResult& InResult)
{
	// 점수 산출 시스템이 아직 없다. 계약이 비어 있으면 구획째로 접어 빈 게이지를 노출하지 않는다.
	const bool bHasScore = InResult.Score >= 0 && InResult.ScoreMax > 0;

	if (Box_Score)
	{
		Box_Score->SetVisibility(
			bHasScore ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (!bHasScore)
	{
		return;
	}

	if (TB_Score)
	{
		TB_Score->SetText(FText::AsNumber(InResult.Score));
	}

	if (Bar_Score)
	{
		Bar_Score->SetPercent(
			FMath::Clamp(static_cast<float>(InResult.Score) / static_cast<float>(InResult.ScoreMax), 0.f, 1.f));
	}
}

void ULastFPSMissionResultWidget::RefreshCombatStats(const FLastFPSMissionCombatStats& InStats)
{
	if (!Box_Stats)
	{
		return;
	}

	Box_Stats->ClearChildren();

	if (!StatEntryClass)
	{
		UE_LOG(LogLastFPSMissionResult, Warning,
			TEXT("MissionResult: StatEntryClass 가 지정되지 않아 전투 통계를 표시하지 못합니다."));
		return;
	}

	// 피해량은 소수점을 보여 줄 이유가 없어 정수로 반올림해 표기한다.
	AddStatRow(DamageDealtLabel, FText::AsNumber(FMath::RoundToInt(InStats.DamageDealt)));
	AddStatRow(DamageTakenLabel, FText::AsNumber(FMath::RoundToInt(InStats.DamageTaken)));
	AddStatRow(KillsLabel, FText::AsNumber(InStats.Kills));
	AddStatRow(DeathsLabel, FText::AsNumber(InStats.Deaths));
	AddStatRow(AssistsLabel, FText::AsNumber(InStats.Assists));
}

void ULastFPSMissionResultWidget::AddStatRow(const FText& InLabel, const FText& InValue)
{
	// 라벨이 비어 있으면 WBP 에서 저작하지 않은 항목이다. 빈 행을 만들지 않고 건너뛴다.
	if (InLabel.IsEmpty() || !Box_Stats || !StatEntryClass)
	{
		return;
	}

	ULastFPSStatEntryWidget* Row = CreateWidget<ULastFPSStatEntryWidget>(this, StatEntryClass);
	if (!Row)
	{
		return;
	}

	Row->SetEntry(InLabel, InValue, NeutralStatSign);
	Box_Stats->AddChild(Row);
}

void ULastFPSMissionResultWidget::RefreshItems(const TArray<FLastFPSItemGrant>& InItems)
{
	if (!WrapBox_Items)
	{
		return;
	}

	WrapBox_Items->ClearChildren();

	int32 NumSlots = 0;

	// 아이템 정의는 GameData 레지스트리의 안정적인 태그 계약으로만 조회한다.
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	const UDataTable* ItemTable =
		GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item) : nullptr;

	if (ItemTable && ItemSlotWidgetClass)
	{
		static const TCHAR* ItemLookupContext = TEXT("ULastFPSMissionResultWidget::RefreshItems");

		for (const FLastFPSItemGrant& Grant : InItems)
		{
			const FLastFPSItemData* ItemDef = ItemTable->FindRow<FLastFPSItemData>(
				Grant.RowId, ItemLookupContext, /*bWarnIfRowMissing=*/false);
			if (!ItemDef)
			{
				// 지급은 됐는데 표시할 정의가 없는 상태. 조용히 넘기면 데이터 오류를 못 찾는다.
				UE_LOG(LogLastFPSMissionResult, Warning,
					TEXT("MissionResult: 획득 아이템 '%s' 를 아이템 테이블에서 찾지 못해 표시를 건너뜁니다."),
					*Grant.RowId.ToString());
				continue;
			}

			ULastFPSItemSlotWidget* ItemSlot =
				CreateWidget<ULastFPSItemSlotWidget>(this, ItemSlotWidgetClass);
			if (!ItemSlot)
			{
				continue;
			}

			ItemSlot->SetupSlot(*ItemDef, Grant.RowId, Grant.Count);
			WrapBox_Items->AddChildToWrapBox(ItemSlot);
			++NumSlots;
		}
	}

	WrapBox_Items->SetVisibility(
		NumSlots > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
