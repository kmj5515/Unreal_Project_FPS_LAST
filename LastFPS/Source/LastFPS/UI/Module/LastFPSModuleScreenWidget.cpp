#include "UI/Module/LastFPSModuleScreenWidget.h"

#include "UI/Module/LastFPSModuleEntryWidget.h"
#include "UI/Module/LastFPSModuleSlotWidget.h"
#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Data/Tables/LastFPSModuleData.h"
#include "Inventory/LastFPSLoadoutSubsystem.h"
#include "Economy/LastFPSEconomySubsystem.h"
#include "Localization/LastFPSLocalization.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"

namespace
{
	FText ModuleStatLabel(ELastFPSModuleStat Stat)
	{
		switch (Stat)
		{
		case ELastFPSModuleStat::MaxHealth:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatHealth);
		case ELastFPSModuleStat::MaxStamina:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatStamina);
		case ELastFPSModuleStat::AttackDamage:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatAttackDamage);
		case ELastFPSModuleStat::Defense:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatDefense);
		case ELastFPSModuleStat::MoveSpeed:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatMoveSpeed);
		default:
			return FText::GetEmpty();
		}
	}

	FText SignedNumber(const float Value)
	{
		FNumberFormattingOptions Options;
		Options.SetMinimumFractionalDigits(0);
		Options.SetMaximumFractionalDigits(1);
		const FText Number = FText::AsNumber(Value, &Options);
		return Value >= 0.f
			? FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::PositiveValueFormat),
				Number)
			: Number;
	}

	/** 모듈 1개의 StatMods → "공격력 +25\n체력 +50" */
	FText FormatStatMods(const TArray<FLastFPSModuleStatMod>& Mods)
	{
		TArray<FText> Lines;
		for (const FLastFPSModuleStatMod& Mod : Mods)
		{
			if (FMath::IsNearlyZero(Mod.Value)) { continue; }
			Lines.Add(FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatLineFormat),
				ModuleStatLabel(Mod.Stat),
				SignedNumber(Mod.Value)));
		}
		return Lines.IsEmpty()
			? FText::GetEmpty()
			: FText::Join(FText::FromString(TEXT("\n")), Lines);
	}

	/** 장착 합산 보정 → 미리보기 텍스트 (0 제외) */
	FText FormatBonusTotals(const FLastFPSModuleStatTotals& T)
	{
		TArray<FText> Lines;
		auto Add = [&Lines](const TCHAR* LabelKey, const float Value)
		{
			if (!FMath::IsNearlyZero(Value))
			{
				Lines.Add(FText::Format(
					FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatLineFormat),
					FLastFPSLocalization::GetUIText(LabelKey),
					SignedNumber(Value)));
			}
		};
		Add(LastFPSUIStringKeys::StatHealth, T.MaxHealth);
		Add(LastFPSUIStringKeys::StatStamina, T.MaxStamina);
		Add(LastFPSUIStringKeys::StatAttackDamage, T.AttackDamage);
		Add(LastFPSUIStringKeys::StatDefense, T.Defense);
		Add(LastFPSUIStringKeys::StatMoveSpeed, T.MoveSpeed);
		return Lines.IsEmpty()
			? FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::NotAvailable)
			: FText::Join(FText::FromString(TEXT("\n")), Lines);
	}
}

void ULastFPSModuleScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULastFPSGameDataSubsystem* GameData = GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>())
		{
			ItemTable = GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item);
		}
	}

	if (ULastFPSLoadoutSubsystem* Loadout = GetLoadout())
	{
		Loadout->OnLoadoutChanged.AddUniqueDynamic(this, &ULastFPSModuleScreenWidget::HandleLoadoutChanged);
	}
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnInventoryChanged.AddUniqueDynamic(this, &ULastFPSModuleScreenWidget::HandleInventoryChanged);
	}

	Rebuild();
}

void ULastFPSModuleScreenWidget::NativeDestruct()
{
	if (ULastFPSLoadoutSubsystem* Loadout = GetLoadout())
	{
		Loadout->OnLoadoutChanged.RemoveDynamic(this, &ULastFPSModuleScreenWidget::HandleLoadoutChanged);
	}
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnInventoryChanged.RemoveDynamic(this, &ULastFPSModuleScreenWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

ULastFPSLoadoutSubsystem* ULastFPSModuleScreenWidget::GetLoadout() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<ULastFPSLoadoutSubsystem>() : nullptr;
}

ULastFPSEconomySubsystem* ULastFPSModuleScreenWidget::GetEconomy() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<ULastFPSEconomySubsystem>() : nullptr;
}

const FLastFPSItemData* ULastFPSModuleScreenWidget::FindItem(FName RowId) const
{
	if (!ItemTable || RowId.IsNone()) { return nullptr; }
	return ItemTable->FindRow<FLastFPSItemData>(RowId, TEXT("ULastFPSModuleScreenWidget::FindItem"), /*bWarnIfRowMissing=*/false);
}

int32 ULastFPSModuleScreenWidget::FindFirstEmptySlot(ULastFPSLoadoutSubsystem* Loadout) const
{
	if (!Loadout) { return INDEX_NONE; }
	for (int32 i = 0; i < Loadout->GetSlotCount(); ++i)
	{
		if (Loadout->GetEquippedModule(i).IsNone()) { return i; }
	}
	return INDEX_NONE;
}

void ULastFPSModuleScreenWidget::HandleLoadoutChanged()  { Rebuild(); }
void ULastFPSModuleScreenWidget::HandleInventoryChanged() { Rebuild(); }

void ULastFPSModuleScreenWidget::Rebuild()
{
	ULastFPSLoadoutSubsystem* Loadout = GetLoadout();
	ULastFPSEconomySubsystem* Econ = GetEconomy();

	RebuildOwned(Loadout, Econ);
	RebuildSlots(Loadout);
	UpdatePreview(Loadout);
}

void ULastFPSModuleScreenWidget::RebuildOwned(ULastFPSLoadoutSubsystem* Loadout, ULastFPSEconomySubsystem* Econ)
{
	if (!Box_OwnedModules || !EntryWidgetClass) { return; }

	Box_OwnedModules->ClearChildren();

	int32 ShownCount = 0;
	if (Loadout && Econ && ItemTable)
	{
		const int32 EmptySlot = FindFirstEmptySlot(Loadout);

		TArray<FName> OwnedIds;
		Econ->GetOwnedItems().GetKeys(OwnedIds);
		OwnedIds.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

		for (const FName& RowId : OwnedIds)
		{
			const int32 Count = Econ->GetItemCount(RowId);
			if (Count <= 0) { continue; }

			// 모듈 정의가 있는 행만 = 모듈 아이템
			const FLastFPSModuleData* Module = Loadout->FindModule(RowId);
			const FLastFPSItemData* Item = FindItem(RowId);
			if (!Module || !Item) { continue; }

			ULastFPSModuleEntryWidget* Entry = CreateWidget<ULastFPSModuleEntryWidget>(this, EntryWidgetClass);
			if (!Entry) { continue; }

			Entry->SetupModule(*Item, FormatStatMods(Module->StatMods), Module->CapacityCost, Count, RowId);
			// 빈 슬롯이 있고 그 슬롯에 캐파 한도 내로 장착 가능할 때만 버튼 활성
			const bool bCanEquip = (EmptySlot != INDEX_NONE) && Loadout->CanEquip(EmptySlot, RowId);
			Entry->SetEquipEnabled(bCanEquip);
			Entry->OnEquipClicked.BindUObject(this, &ULastFPSModuleScreenWidget::HandleEquipClicked);

			Box_OwnedModules->AddChild(Entry);
			++ShownCount;
		}
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(ShownCount > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void ULastFPSModuleScreenWidget::RebuildSlots(ULastFPSLoadoutSubsystem* Loadout)
{
	if (!Box_Slots || !SlotWidgetClass || !Loadout) { return; }

	Box_Slots->ClearChildren();

	for (int32 i = 0; i < Loadout->GetSlotCount(); ++i)
	{
		ULastFPSModuleSlotWidget* SlotWidget = CreateWidget<ULastFPSModuleSlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget) { continue; }

		const FName RowId = Loadout->GetEquippedModule(i);
		const FLastFPSModuleData* Module = Loadout->FindModule(RowId);
		const FLastFPSItemData* Item = FindItem(RowId);
		if (Module && Item)
		{
			SlotWidget->SetupEquipped(i, *Item, FormatStatMods(Module->StatMods));
		}
		else
		{
			SlotWidget->SetEmpty(i);
		}

		SlotWidget->OnSlotClicked.BindUObject(this, &ULastFPSModuleScreenWidget::HandleSlotClicked);
		Box_Slots->AddChild(SlotWidget);
	}
}

void ULastFPSModuleScreenWidget::UpdatePreview(ULastFPSLoadoutSubsystem* Loadout)
{
	if (!Loadout) { return; }

	if (TB_Capacity)
	{
		TB_Capacity->SetText(FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::RatioFormat),
			FText::AsNumber(Loadout->GetUsedCapacity()),
			FText::AsNumber(Loadout->GetMaxCapacity())));
	}

	if (TB_StatPreview)
	{
		TB_StatPreview->SetText(FormatBonusTotals(Loadout->ComputeBonus()));
	}
}

void ULastFPSModuleScreenWidget::HandleEquipClicked(FName RowId)
{
	ULastFPSLoadoutSubsystem* Loadout = GetLoadout();
	if (!Loadout) { return; }

	const int32 TargetSlot = FindFirstEmptySlot(Loadout);
	if (TargetSlot == INDEX_NONE || !Loadout->TryEquip(TargetSlot, RowId))
	{
		OnEquipRejected();
		return;
	}
	// 성공 시 LoadoutSubsystem 이 OnLoadoutChanged 브로드캐스트 → Rebuild
}

void ULastFPSModuleScreenWidget::HandleSlotClicked(int32 SlotIndex)
{
	if (ULastFPSLoadoutSubsystem* Loadout = GetLoadout())
	{
		Loadout->Unequip(SlotIndex);
	}
}
