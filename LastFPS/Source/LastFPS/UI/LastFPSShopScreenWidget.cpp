#include "UI/LastFPSShopScreenWidget.h"

#include "UI/LastFPSShopEntryWidget.h"
#include "Shop/LastFPSShopData.h"
#include "Inventory/LastFPSItemData.h"
#include "Economy/LastFPSEconomySubsystem.h"
#include "Game/LastFPSPlayerController.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"

void ULastFPSShopScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildShopList();

	// 잔액/보유 변동 구독 + 최초 1회 동기화(잔액 표시 + 구매 가능 여부)
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnCreditsChanged.AddDynamic(this, &ULastFPSShopScreenWidget::HandleCreditsChanged);
		Econ->OnInventoryChanged.AddDynamic(this, &ULastFPSShopScreenWidget::HandleInventoryChanged);
		HandleCreditsChanged(Econ->GetCredits());
	}
}

void ULastFPSShopScreenWidget::NativeDestruct()
{
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnCreditsChanged.RemoveDynamic(this, &ULastFPSShopScreenWidget::HandleCreditsChanged);
		Econ->OnInventoryChanged.RemoveDynamic(this, &ULastFPSShopScreenWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

ULastFPSEconomySubsystem* ULastFPSShopScreenWidget::GetEconomy() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<ULastFPSEconomySubsystem>() : nullptr;
}

void ULastFPSShopScreenWidget::RebuildShopList()
{
	if (!Box_ShopList)
	{
		return;
	}

	Box_ShopList->ClearChildren();
	EntryByRow.Reset();

	int32 NumRows = 0;

	if (ShopTable && EntryWidgetClass)
	{
		// DataTable 행 순서대로 엔트리 생성. (재고 무제한 — 정렬/필터는 추후)
		ShopTable->ForeachRow<FLastFPSShopItemData>(TEXT("ULastFPSShopScreenWidget::RebuildShopList"),
			[this, &NumRows](const FName& RowName, const FLastFPSShopItemData& Row)
			{
				ULastFPSShopEntryWidget* Entry = CreateWidget<ULastFPSShopEntryWidget>(this, EntryWidgetClass);
				if (!Entry)
				{
					return;
				}

				Entry->SetupShopItem(Row, RowName);
				Entry->OnBuyClicked.BindUObject(this, &ULastFPSShopScreenWidget::HandleBuyRequested, Entry);
				Box_ShopList->AddChild(Entry);
				EntryByRow.Add(RowName, Entry);
				++NumRows;
			});
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(NumRows > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	// 새로 만든 엔트리들의 구매 가능 여부를 현재 잔액으로 맞춤
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		HandleCreditsChanged(Econ->GetCredits());
	}
}

int32 ULastFPSShopScreenWidget::ComputeMaxPurchasable(const FLastFPSShopItemData& Row) const
{
	const ULastFPSEconomySubsystem* Econ = GetEconomy();
	if (!Econ)
	{
		return 0;
	}

	// 잔액 한도 (단가 0 = 무료면 사실상 무제한)
	const int32 UnitPrice   = FMath::Max(0, Row.Price);
	const int32 AffordLimit = (UnitPrice > 0) ? (Econ->GetCredits() / UnitPrice) : MAX_int32;

	// 지급 아이템이 없으면(화폐성 구매) 스택 제약 없음 → 잔액 한도만
	if (Row.GrantItemRowId.IsNone())
	{
		return FMath::Max(0, AffordLimit);
	}

	// 스택 여유 = MaxStackSize - 현재 보유. (ItemTable 미설정/행 없음 시 1로 가정 → 무기류 1회 제한)
	int32 MaxStack = 1;
	if (ItemTable)
	{
		if (const FLastFPSItemData* Item = ItemTable->FindRow<FLastFPSItemData>(Row.GrantItemRowId, TEXT("ULastFPSShopScreenWidget::ComputeMaxPurchasable"), /*bWarnIfRowMissing=*/false))
		{
			MaxStack = FMath::Max(1, Item->MaxStackSize);
		}
	}
	const int32 StackRoom = MaxStack - Econ->GetItemCount(Row.GrantItemRowId);

	return FMath::Max(0, FMath::Min(AffordLimit, StackRoom));
}

void ULastFPSShopScreenWidget::HandleBuyRequested(FName RowName, ULastFPSShopEntryWidget* Entry)
{
	if (!ShopTable)
	{
		return;
	}

	const FLastFPSShopItemData* Row = ShopTable->FindRow<FLastFPSShopItemData>(RowName, TEXT("ULastFPSShopScreenWidget::HandleBuyRequested"), /*bWarnIfRowMissing=*/false);
	if (!Row)
	{
		return;
	}

	ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>();
	if (!PC)
	{
		return;
	}

	const int32 MaxQty = ComputeMaxPurchasable(*Row);
	if (MaxQty <= 0)
	{
		// 잔액 부족이거나 더 보유할 수 없는(스택이 찬) 아이템
		PC->ShowNotice(
			NSLOCTEXT("LastFPSShop", "CannotBuyTitle", "구매 불가"),
			NSLOCTEXT("LastFPSShop", "CannotBuyBody", "잔액이 부족하거나 더 보유할 수 없는 아이템입니다."));
		return;
	}

	// 구매 대상 보관 후 수량 선택 모달 표시 (모달은 한 번에 하나)
	PendingRowName = RowName;
	PendingEntry   = Entry;

	FLastFPSQuantityResultDelegate OnResult;
	OnResult.BindDynamic(this, &ULastFPSShopScreenWidget::HandleQuantityChosen);
	PC->ShowQuantityPrompt(
		NSLOCTEXT("LastFPSShop", "BuyConfirmTitle", "구매하시겠습니까?"),
		Row->ItemName,
		FMath::Max(0, Row->Price),
		MaxQty,
		OnResult);
}

void ULastFPSShopScreenWidget::HandleQuantityChosen(int32 Quantity)
{
	ULastFPSShopEntryWidget* Entry = PendingEntry.Get();
	const FName RowName = PendingRowName;
	PendingRowName = NAME_None;
	PendingEntry   = nullptr;

	if (Quantity <= 0)
	{
		return; // 취소
	}

	ULastFPSEconomySubsystem* Econ = GetEconomy();
	if (!Econ || !ShopTable)
	{
		return;
	}

	const FLastFPSShopItemData* Row = ShopTable->FindRow<FLastFPSShopItemData>(RowName, TEXT("ULastFPSShopScreenWidget::HandleQuantityChosen"), /*bWarnIfRowMissing=*/false);
	if (!Row)
	{
		return;
	}

	// 잔액 충분하면 단가×수량 차감 + 수량만큼 지급. 변동은 OnCreditsChanged/OnInventoryChanged 로 버튼 상태 갱신.
	if (Econ->TryPurchase(Row->GrantItemRowId, Row->Price, Quantity) && Entry)
	{
		Entry->OnPurchaseSucceeded();
	}
}

void ULastFPSShopScreenWidget::HandleCreditsChanged(int32 NewCredits)
{
	if (TB_Credits)
	{
		TB_Credits->SetText(FText::AsNumber(NewCredits));
	}
	RefreshEntryStates();
}

void ULastFPSShopScreenWidget::HandleInventoryChanged()
{
	RefreshEntryStates();
}

void ULastFPSShopScreenWidget::RefreshEntryStates()
{
	if (!ShopTable)
	{
		return;
	}

	// 각 엔트리: 잔액 + 스택 여유로 한 개라도 살 수 있으면 구매 버튼 활성
	for (const TPair<FName, TObjectPtr<ULastFPSShopEntryWidget>>& Pair : EntryByRow)
	{
		if (!Pair.Value)
		{
			continue;
		}
		const FLastFPSShopItemData* Row = ShopTable->FindRow<FLastFPSShopItemData>(Pair.Key, TEXT("ULastFPSShopScreenWidget::RefreshEntryStates"), /*bWarnIfRowMissing=*/false);
		Pair.Value->SetAffordable(Row && ComputeMaxPurchasable(*Row) > 0);
	}
}
