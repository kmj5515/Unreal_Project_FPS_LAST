#include "UI/LastFPSInventoryWidget.h"

#include "UI/LastFPSItemSlotWidget.h"
#include "Inventory/LastFPSItemData.h"
#include "Economy/LastFPSEconomySubsystem.h"

#include "Components/PanelWidget.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"

void ULastFPSInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnInventoryChanged.AddDynamic(this, &ULastFPSInventoryWidget::HandleInventoryChanged);
	}

	RebuildInventory();
}

void ULastFPSInventoryWidget::NativeDestruct()
{
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnInventoryChanged.RemoveDynamic(this, &ULastFPSInventoryWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

ULastFPSEconomySubsystem* ULastFPSInventoryWidget::GetEconomy() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<ULastFPSEconomySubsystem>() : nullptr;
}

void ULastFPSInventoryWidget::HandleInventoryChanged()
{
	RebuildInventory();
}

void ULastFPSInventoryWidget::RebuildInventory()
{
	if (!Box_Slots || !SlotWidgetClass)
	{
		return;
	}

	Box_Slots->ClearChildren();

	// 보유 아이템(RowId→Count)을 ItemTable 정의로 해석. RowId 안정 정렬로 표시 순서 고정.
	TArray<TPair<const FLastFPSItemData*, int32>> OwnedRows;
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		if (ItemTable)
		{
			TArray<FName> OwnedIds;
			Econ->GetOwnedItems().GetKeys(OwnedIds);
			OwnedIds.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

			for (const FName& RowId : OwnedIds)
			{
				const int32 Count = Econ->GetItemCount(RowId);
				if (Count <= 0)
				{
					continue;
				}

				if (const FLastFPSItemData* Row = ItemTable->FindRow<FLastFPSItemData>(RowId, TEXT("ULastFPSInventoryWidget::RebuildInventory"), /*bWarnIfRowMissing=*/false))
				{
					OwnedRows.Add(TPair<const FLastFPSItemData*, int32>(Row, Count));
				}
			}
		}
	}

	// SlotCount 개 슬롯 생성 — 앞에서부터 보유 아이템으로 채우고 나머지는 빈 슬롯
	for (int32 i = 0; i < SlotCount; ++i)
	{
		ULastFPSItemSlotWidget* SlotWidget = CreateWidget<ULastFPSItemSlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		if (OwnedRows.IsValidIndex(i))
		{
			SlotWidget->SetupSlot(*OwnedRows[i].Key, OwnedRows[i].Value);
		}
		else
		{
			SlotWidget->SetEmpty();
		}

		Box_Slots->AddChild(SlotWidget);
	}
}
