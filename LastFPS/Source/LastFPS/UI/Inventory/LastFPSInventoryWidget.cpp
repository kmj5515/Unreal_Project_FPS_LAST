#include "UI/Inventory/LastFPSInventoryWidget.h"

#include "UI/Inventory/LastFPSItemSlotWidget.h"
#include "Data/Tables/LastFPSItemData.h"
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

void ULastFPSInventoryWidget::SetAllowedTypes(const TArray<ELastFPSItemType>& InTypes)
{
	AllowedTypes = InTypes;
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
	TArray<TTuple<FName, const FLastFPSItemData*, int32>> OwnedRows;
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
					// 카테고리 탭 필터 — AllowedTypes 가 비어 있으면 전체 표시.
					if (AllowedTypes.Num() > 0 && !AllowedTypes.Contains(Row->ItemType))
					{
						continue;
					}

					OwnedRows.Add(MakeTuple(RowId, Row, Count));
				}
			}
		}
	}

	// 보유 아이템 개수만큼만 슬롯 생성 (빈 칸 패딩 없음)
	for (const TTuple<FName, const FLastFPSItemData*, int32>& Owned : OwnedRows)
	{
		ULastFPSItemSlotWidget* SlotWidget = CreateWidget<ULastFPSItemSlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->OnHovered.BindUObject(this, &ULastFPSInventoryWidget::HandleSlotHovered);
		SlotWidget->OnUnhovered.BindUObject(this, &ULastFPSInventoryWidget::HandleSlotUnhovered);
		SlotWidget->SetupSlot(*Owned.Get<1>(), Owned.Get<0>(), Owned.Get<2>());
		Box_Slots->AddChild(SlotWidget);
	}
}

void ULastFPSInventoryWidget::HandleSlotHovered(FName RowId)
{
	HoveredItemRowId = RowId;
}

void ULastFPSInventoryWidget::HandleSlotUnhovered(FName RowId)
{
	// 다른 슬롯으로 이미 이동한 경우(다음 슬롯 Enter 가 먼저 온 경우)는 덮어쓰지 않는다.
	if (HoveredItemRowId == RowId)
	{
		HoveredItemRowId = NAME_None;
	}
}
