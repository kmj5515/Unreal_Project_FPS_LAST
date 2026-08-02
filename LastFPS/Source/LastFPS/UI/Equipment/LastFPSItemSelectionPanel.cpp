#include "UI/Equipment/LastFPSItemSelectionPanel.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Economy/LastFPSEconomySubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Localization/LastFPSLocalization.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Inventory/LastFPSItemSlotWidget.h"

void ULastFPSItemSelectionPanel::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked().AddUObject(this, &ULastFPSItemSelectionPanel::HandleConfirmClicked);
	}

	if (Button_Cancel)
	{
		Button_Cancel->OnClicked().AddUObject(this, &ULastFPSItemSelectionPanel::HandleCancelClicked);
	}

	Close();
}

ULastFPSEquipmentSubsystem* ULastFPSItemSelectionPanel::GetEquipment() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>() : nullptr;
}

void ULastFPSItemSelectionPanel::OpenForSlot(
	const ELastFPSEquipmentSlotType InSlotType, const int32 InSlotIndex)
{
	TargetSlotType  = InSlotType;
	TargetSlotIndex = InSlotIndex;

	// 이전에 열었을 때의 선택이 남아 다른 슬롯의 비교에 섞이지 않게 초기화한다.
	SelectedItemRowId = NAME_None;

	SetVisibility(ESlateVisibility::Visible);
	RebuildCandidates();
	UpdateComparison();
}

void ULastFPSItemSelectionPanel::Close()
{
	SelectedItemRowId = NAME_None;
	TargetSlotIndex   = INDEX_NONE;
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULastFPSItemSelectionPanel::RebuildCandidates()
{
	if (Box_Candidates)
	{
		Box_Candidates->ClearChildren();
	}
	EntryWidgets.Reset();

	const ULastFPSEquipmentSubsystem* Equipment = GetEquipment();
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSEconomySubsystem* Economy =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSEconomySubsystem>() : nullptr;
	ULastFPSGameDataSubsystem* GameData =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	const UDataTable* ItemTable =
		GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item) : nullptr;

	if (!Equipment || !Economy || !ItemTable || !Box_Candidates || !EntryWidgetClass)
	{
		if (TB_Empty)
		{
			TB_Empty->SetText(EmptyCandidateText);
			TB_Empty->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		return;
	}

	const ELastFPSItemType AcceptedType = Equipment->GetAcceptedItemType(TargetSlotType);

	static const FString Context(TEXT("ULastFPSItemSelectionPanel::RebuildCandidates"));

	// 보유 아이템 중 이 카테고리에 맞는 것만 모아 희귀도 내림차순 → 이름 순으로 정렬한다.
	struct FCandidate
	{
		FName RowId;
		const FLastFPSItemData* Item;
		int32 Count;
	};

	TArray<FCandidate> Candidates;
	for (const TPair<FName, int32>& OwnedItem : Economy->GetOwnedItems())
	{
		if (OwnedItem.Value <= 0)
		{
			continue;
		}

		const FLastFPSItemData* Item =
			ItemTable->FindRow<FLastFPSItemData>(OwnedItem.Key, Context, /*bWarnIfRowMissing=*/false);
		if (!Item || Item->ItemType != AcceptedType)
		{
			continue;
		}

		Candidates.Add({ OwnedItem.Key, Item, OwnedItem.Value });
	}

	Candidates.Sort([](const FCandidate& A, const FCandidate& B)
	{
		if (A.Item->Rarity != B.Item->Rarity)
		{
			return A.Item->Rarity > B.Item->Rarity;
		}
		return A.Item->ItemName.CompareTo(B.Item->ItemName) < 0;
	});

	for (const FCandidate& Candidate : Candidates)
	{
		ULastFPSItemSlotWidget* Entry =
			CreateWidget<ULastFPSItemSlotWidget>(GetOwningPlayer(), EntryWidgetClass);
		if (!Entry)
		{
			continue;
		}

		Entry->SetupSlot(*Candidate.Item, Candidate.RowId, Candidate.Count);
		const bool bCanEquip = Equipment->CanEquip(
			TargetSlotType,
			TargetSlotIndex,
			Candidate.RowId);
		Entry->SetIsEnabled(bCanEquip);
		Entry->SetRenderOpacity(
			bCanEquip ? 1.f : FMath::Clamp(UnequippableOpacity, 0.f, 1.f));

		Entry->OnClicked.BindUObject(this, &ULastFPSItemSelectionPanel::HandleEntrySelected);
		Entry->OnDoubleClicked.BindUObject(this, &ULastFPSItemSelectionPanel::HandleEntryChosen);

		Box_Candidates->AddChild(Entry);
		EntryWidgets.Add(Entry);
	}

	if (TB_Empty)
	{
		const bool bHasCandidates = EntryWidgets.Num() > 0;
		TB_Empty->SetText(EmptyCandidateText);
		TB_Empty->SetVisibility(bHasCandidates ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (Button_Confirm)
	{
		Button_Confirm->SetIsEnabled(false);
	}
}

void ULastFPSItemSelectionPanel::HandleEntrySelected(const FName ItemRowId)
{
	SelectedItemRowId = ItemRowId;

	for (ULastFPSItemSlotWidget* Entry : EntryWidgets)
	{
		if (Entry)
		{
			Entry->SetSelected(Entry->GetItemRowId() == ItemRowId);
		}
	}

	if (Button_Confirm)
	{
		const ULastFPSEquipmentSubsystem* Equipment = GetEquipment();
		Button_Confirm->SetIsEnabled(
			Equipment && Equipment->CanEquip(TargetSlotType, TargetSlotIndex, ItemRowId));
	}

	UpdateComparison();
}

void ULastFPSItemSelectionPanel::HandleEntryChosen(const FName ItemRowId)
{
	OnItemChosen.ExecuteIfBound(TargetSlotType, TargetSlotIndex, ItemRowId);
}

void ULastFPSItemSelectionPanel::HandleConfirmClicked()
{
	if (!SelectedItemRowId.IsNone())
	{
		OnItemChosen.ExecuteIfBound(TargetSlotType, TargetSlotIndex, SelectedItemRowId);
	}
}

void ULastFPSItemSelectionPanel::HandleCancelClicked()
{
	Close();
}

void ULastFPSItemSelectionPanel::UpdateComparison()
{
	if (!TB_StatComparison)
	{
		return;
	}

	const ULastFPSEquipmentSubsystem* Equipment = GetEquipment();
	if (!Equipment || SelectedItemRowId.IsNone())
	{
		TB_StatComparison->SetText(FText::GetEmpty());
		return;
	}

	const FLastFPSEquipmentStatTotals Current = Equipment->ComputeTotals();
	const FLastFPSEquipmentStatTotals Candidate =
		Equipment->ComputeTotalsWithCandidate(TargetSlotType, TargetSlotIndex, SelectedItemRowId);

	// 변화가 있는 스탯만 한 줄씩 쌓는다. 전체 스탯을 항상 나열하면 무엇이 바뀌었는지 읽기 어렵다.
	TArray<FText> Lines;
	for (int32 StatIndex = 0; StatIndex < static_cast<int32>(ELastFPSEquipmentStat::Count); ++StatIndex)
	{
		const ELastFPSEquipmentStat Stat = static_cast<ELastFPSEquipmentStat>(StatIndex);
		const float Delta = Candidate.GetStat(Stat) - Current.GetStat(Stat);
		if (FMath::IsNearlyZero(Delta))
		{
			continue;
		}

		// 색은 RichText 마크업이 아닌 접두 기호로 표현해, 일반 TextBlock 에서도 방향이 읽히게 한다.
		const FText Arrow = FText::FromString(Delta > 0.f ? TEXT("▲") : TEXT("▼"));
		Lines.Add(FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatComparisonLineFormat),
			Arrow,
			LastFPSEquipmentStats::GetDisplayName(Stat),
			LastFPSEquipmentStats::FormatValue(Stat, Delta, /*bShowSign=*/true)));
	}

	TB_StatComparison->SetText(FText::Join(FText::FromString(TEXT("\n")), Lines));
}
