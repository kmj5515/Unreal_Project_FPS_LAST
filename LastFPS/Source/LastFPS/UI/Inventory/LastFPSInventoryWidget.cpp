#include "UI/Inventory/LastFPSInventoryWidget.h"

#include "UI/Inventory/LastFPSItemSlotWidget.h"
#include "UI/Inventory/LastFPSWeaponPreviewWidget.h"
#include "UI/Inventory/LastFPSWeaponPreviewRig.h"
#include "UI/Framework/LastFPSUITags.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Economy/LastFPSEconomySubsystem.h"

#include "Components/PanelWidget.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "PrimaryGameLayout.h"

void ULastFPSInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnInventoryChanged.AddDynamic(this, &ULastFPSInventoryWidget::HandleInventoryChanged);
	}

	RebuildInventory();

	// 첫 프리뷰 흰색 방지 — 인벤토리 열 때 리그를 미리 스폰·예열해 둔다.
	EnsurePreviewRig();
}

void ULastFPSInventoryWidget::NativeDestruct()
{
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnInventoryChanged.RemoveDynamic(this, &ULastFPSInventoryWidget::HandleInventoryChanged);
	}

	if (PreviewRig)
	{
		PreviewRig->Destroy();
		PreviewRig = nullptr;
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

ALastFPSWeaponPreviewRig* ULastFPSInventoryWidget::EnsurePreviewRig()
{
	if (PreviewRig)
	{
		return PreviewRig;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TSubclassOf<ALastFPSWeaponPreviewRig> RigClass = PreviewRigClass;
	if (!RigClass)
	{
		RigClass = ALastFPSWeaponPreviewRig::StaticClass();
	}
	PreviewRig = World->SpawnActor<ALastFPSWeaponPreviewRig>(
		RigClass, FVector(0.f, 0.f, 100000.f), FRotator::ZeroRotator, Params);
	if (!PreviewRig)
	{
		return nullptr;
	}

	// 보유 무기 첫 스켈레탈 메시로 캡처를 미리 데워 둔다(있으면). 실제 무기는 F1 때 교체된다.
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		if (ItemTable)
		{
			TArray<FName> OwnedIds;
			Econ->GetOwnedItems().GetKeys(OwnedIds);
			for (const FName& Id : OwnedIds)
			{
				const FLastFPSItemData* Row = ItemTable->FindRow<FLastFPSItemData>(Id, TEXT("ULastFPSInventoryWidget::EnsurePreviewRig"), /*bWarnIfRowMissing=*/false);
				if (Row && Row->ItemType == ELastFPSItemType::Weapon)
				{
					if (ULastFPSWeaponDefinition* Def = Row->WeaponDefinition.LoadSynchronous())
					{
						if (Def->SkeletalMesh)
						{
							PreviewRig->InitPreview(Def->SkeletalMesh);
							break;
						}
					}
				}
			}
		}
	}

	return PreviewRig;
}

FReply ULastFPSInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::F1)
	{
		if (TryOpenWeaponPreview())
		{
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool ULastFPSInventoryWidget::TryOpenWeaponPreview()
{
	if (HoveredItemRowId.IsNone() || !ItemTable || !PreviewWidgetClass)
	{
		return false;
	}

	const FLastFPSItemData* Row = ItemTable->FindRow<FLastFPSItemData>(
		HoveredItemRowId, TEXT("ULastFPSInventoryWidget::TryOpenWeaponPreview"), /*bWarnIfRowMissing=*/false);
	if (!Row || Row->ItemType != ELastFPSItemType::Weapon)
	{
		return false;
	}

	// 무기 정의가 연결돼 있어야 3D/스탯을 보여줄 수 있다.
	ULastFPSWeaponDefinition* Def = Row->WeaponDefinition.LoadSynchronous();
	if (!Def)
	{
		return false;
	}

	APlayerController* PC = GetOwningPlayer();
	UPrimaryGameLayout* Layout = PC ? UPrimaryGameLayout::GetPrimaryGameLayout(PC) : nullptr;
	if (!Layout)
	{
		return false;
	}

	// Modal 레이어에 프리뷰 오버레이 push (인벤토리는 아래 Menu 레이어에 그대로 남음).
	const FLastFPSItemData ItemCopy = *Row;
	const FName RowId = HoveredItemRowId;
	ALastFPSWeaponPreviewRig* Rig = EnsurePreviewRig();
	Layout->PushWidgetToLayerStack<ULastFPSWeaponPreviewWidget>(
		LastFPSUITags::Layer_Modal(),
		PreviewWidgetClass,
		[Def, ItemCopy, RowId, Rig](ULastFPSWeaponPreviewWidget& Widget)
		{
			Widget.Setup(Def, ItemCopy, RowId, Rig);
		});

	return true;
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
	// 의도적으로 마지막 호버를 유지 — 모달 프리뷰가 열리면 슬롯이 leave 를 받는데
	// 여기서 지우면 프리뷰를 닫고 다시 F1 할 때 대상이 없어 안 열린다.
}
