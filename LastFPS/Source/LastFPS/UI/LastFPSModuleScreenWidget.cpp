#include "UI/LastFPSModuleScreenWidget.h"

#include "UI/LastFPSModuleEntryWidget.h"
#include "UI/LastFPSModuleSlotWidget.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Data/Tables/LastFPSModuleData.h"
#include "Inventory/LastFPSLoadoutSubsystem.h"
#include "Economy/LastFPSEconomySubsystem.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"

namespace
{
	FString ModuleStatLabel(ELastFPSModuleStat Stat)
	{
		switch (Stat)
		{
		case ELastFPSModuleStat::MaxHealth:    return TEXT("체력");
		case ELastFPSModuleStat::MaxStamina:   return TEXT("스태미나");
		case ELastFPSModuleStat::AttackDamage: return TEXT("공격력");
		case ELastFPSModuleStat::Defense:      return TEXT("방어력");
		case ELastFPSModuleStat::MoveSpeed:    return TEXT("이동속도");
		default:                               return TEXT("");
		}
	}

	FString SignedNumber(float Value)
	{
		const FString Num = FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value))
			? FString::FromInt(FMath::RoundToInt(Value))
			: FString::Printf(TEXT("%.1f"), Value);
		return (Value >= 0.f ? TEXT("+") : TEXT("")) + Num;
	}

	/** 모듈 1개의 StatMods → "공격력 +25\n체력 +50" */
	FText FormatStatMods(const TArray<FLastFPSModuleStatMod>& Mods)
	{
		TArray<FString> Lines;
		for (const FLastFPSModuleStatMod& Mod : Mods)
		{
			if (FMath::IsNearlyZero(Mod.Value)) { continue; }
			Lines.Add(FString::Printf(TEXT("%s %s"), *ModuleStatLabel(Mod.Stat), *SignedNumber(Mod.Value)));
		}
		return FText::FromString(FString::Join(Lines, TEXT("\n")));
	}

	/** 장착 합산 보정 → 미리보기 텍스트 (0 제외) */
	FText FormatBonusTotals(const FLastFPSModuleStatTotals& T)
	{
		TArray<FString> Lines;
		auto Add = [&Lines](const TCHAR* Label, float V)
		{
			if (!FMath::IsNearlyZero(V)) { Lines.Add(FString::Printf(TEXT("%s %s"), Label, *SignedNumber(V))); }
		};
		Add(TEXT("체력"),      T.MaxHealth);
		Add(TEXT("스태미나"),  T.MaxStamina);
		Add(TEXT("공격력"),    T.AttackDamage);
		Add(TEXT("방어력"),    T.Defense);
		Add(TEXT("이동속도"),  T.MoveSpeed);
		return Lines.Num() > 0 ? FText::FromString(FString::Join(Lines, TEXT("\n"))) : FText::FromString(TEXT("—"));
	}
}

void ULastFPSModuleScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULastFPSLoadoutSubsystem* Loadout = GetLoadout())
	{
		Loadout->OnLoadoutChanged.AddDynamic(this, &ULastFPSModuleScreenWidget::HandleLoadoutChanged);
	}
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnInventoryChanged.AddDynamic(this, &ULastFPSModuleScreenWidget::HandleInventoryChanged);
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
		TB_Capacity->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), Loadout->GetUsedCapacity(), Loadout->GetMaxCapacity())));
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
