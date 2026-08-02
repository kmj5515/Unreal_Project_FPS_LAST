#include "UI/Inventory/LastFPSInventoryWidget.h"

#include "UI/Inventory/LastFPSItemSlotWidget.h"
#include "UI/Inventory/LastFPSWeaponPreviewWidget.h"
#include "UI/Preview/LastFPSPreviewStageActor.h"
#include "UI/Preview/LastFPSPreviewStageSubsystem.h"
#include "UI/Framework/LastFPSUITags.h"
#include "UI/Framework/LastFPSUIManagerSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Economy/LastFPSEconomySubsystem.h"

#include "Components/PanelWidget.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "CommonActivatableWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSInventory, Log, All);

void ULastFPSInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULastFPSGameDataSubsystem* GameData = GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>())
		{
			ItemTable = GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item);
		}
	}

	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnInventoryChanged.AddUniqueDynamic(this, &ULastFPSInventoryWidget::HandleInventoryChanged);
	}

	RebuildInventory();

	// 프리뷰 무대는 레벨 시작에 이미 스폰돼 있다(ULastFPSPreviewStageSubsystem).
	// 여기서 스폰하거나 예열하면 인벤토리를 여는 프레임에 무기 정의·메시 로드 비용을 그대로 낸다.
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

FReply ULastFPSInventoryWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 툴팁이 떠 있다는 것은 hover 중인 아이템이 있다는 뜻이다. 그 상태면 어느 자식이 포커스를 쥐고 있든
	// 같은 키가 같은 동작을 해야 한다. 열지 못하면 원래 경로로 넘겨 다른 위젯이 처리하게 둔다.
	if (WeaponPreviewKey.IsValid() && InKeyEvent.GetKey() == WeaponPreviewKey)
	{
		if (TryOpenWeaponPreview())
		{
			return FReply::Handled();
		}
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

bool ULastFPSInventoryWidget::TryOpenWeaponPreview()
{
	if (HoveredItemRowId.IsNone() || !ItemTable)
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

	ULastFPSUIManagerSubsystem* UIManager = ULastFPSUIManagerSubsystem::Get(this);
	if (!UIManager)
	{
		return false;
	}

	const FLastFPSItemData ItemCopy = *Row;
	const FName RowId = HoveredItemRowId;

	// 무대는 레벨이 소유한다. 화면은 빌려 쓰기만 하므로 여기서 스폰하거나 파괴하지 않는다.
	ULastFPSPreviewStageSubsystem* PreviewStageSubsystem = ULastFPSPreviewStageSubsystem::Get(this);
	ALastFPSPreviewStageActor* PreviewStage = PreviewStageSubsystem ? PreviewStageSubsystem->GetStage() : nullptr;
	if (!PreviewStage)
	{
		UE_LOG(LogLastFPSInventory, Warning,
			TEXT("%s: 프리뷰 무대가 준비되지 않아 프리뷰를 열지 못했습니다."), *GetName());
		return false;
	}

	// 레이어·위젯 클래스·프리뷰 시점은 화면 정의가 정한다. 인벤토리는 무엇을 보여줄지만 넘긴다.
	return UIManager->OpenScreenWithInit(
		LastFPSUITags::Screen_WeaponPreview(),
		GetOwningPlayer(),
		[Def, ItemCopy, RowId, PreviewStage](UCommonActivatableWidget& Widget)
		{
			if (ULastFPSWeaponPreviewWidget* PreviewWidget = Cast<ULastFPSWeaponPreviewWidget>(&Widget))
			{
				PreviewWidget->Setup(Def, ItemCopy, RowId, PreviewStage);
			}
		}) != nullptr;
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

				if (const FLastFPSItemData* Row = ItemTable->FindRow<FLastFPSItemData>(
					RowId,
					TEXT("ULastFPSInventoryWidget::RebuildInventory"),
					/*bWarnIfRowMissing=*/false))
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
		
		SlotWidget->SetPadding(FMargin(20.0f));
		
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
