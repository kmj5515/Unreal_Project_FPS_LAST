#include "UI/LastFPSInventoryWidget.h"

#include "UI/LastFPSItemSlotWidget.h"
#include "Inventory/LastFPSItemData.h"

#include "Components/PanelWidget.h"
#include "Engine/DataTable.h"

void ULastFPSInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildInventory();
}

void ULastFPSInventoryWidget::RebuildInventory()
{
	if (!Box_Slots || !SlotWidgetClass)
	{
		return;
	}

	Box_Slots->ClearChildren();

	// DataTable 행을 순서대로 수집
	TArray<FLastFPSItemData*> Rows;
	if (ItemTable)
	{
		ItemTable->GetAllRows<FLastFPSItemData>(TEXT("ULastFPSInventoryWidget::RebuildInventory"), Rows);
	}

	// SlotCount 개 슬롯 생성 — 앞에서부터 아이템으로 채우고 나머지는 빈 슬롯
	for (int32 i = 0; i < SlotCount; ++i)
	{
		ULastFPSItemSlotWidget* Slot = CreateWidget<ULastFPSItemSlotWidget>(this, SlotWidgetClass);
		if (!Slot)
		{
			continue;
		}

		if (Rows.IsValidIndex(i) && Rows[i])
		{
			Slot->SetupSlot(*Rows[i]);
		}
		else
		{
			Slot->SetEmpty();
		}

		Box_Slots->AddChild(Slot);
	}
}
