#include "UI/LastFPSShopScreenWidget.h"

#include "UI/LastFPSShopEntryWidget.h"
#include "Shop/LastFPSShopData.h"
#include "Economy/LastFPSEconomySubsystem.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"

void ULastFPSShopScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildShopList();

	// 잔액 변동 구독 + 최초 1회 동기화(잔액 표시 + 구매 가능 여부)
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnCreditsChanged.AddDynamic(this, &ULastFPSShopScreenWidget::HandleCreditsChanged);
		HandleCreditsChanged(Econ->GetCredits());
	}
}

void ULastFPSShopScreenWidget::NativeDestruct()
{
	if (ULastFPSEconomySubsystem* Econ = GetEconomy())
	{
		Econ->OnCreditsChanged.RemoveDynamic(this, &ULastFPSShopScreenWidget::HandleCreditsChanged);
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
				Entry->OnBuyClicked.BindUObject(this, &ULastFPSShopScreenWidget::HandleItemPurchased, Entry);
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

void ULastFPSShopScreenWidget::HandleItemPurchased(FName RowName, ULastFPSShopEntryWidget* Entry)
{
	ULastFPSEconomySubsystem* Econ = GetEconomy();
	if (!Econ || !ShopTable)
	{
		return;
	}

	const FLastFPSShopItemData* Row = ShopTable->FindRow<FLastFPSShopItemData>(RowName, TEXT("ULastFPSShopScreenWidget::HandleItemPurchased"), /*bWarnIfRowMissing=*/false);
	if (!Row)
	{
		return;
	}

	// 잔액 충분하면 차감 + 아이템 지급. 잔액 변동은 OnCreditsChanged 로 버튼 상태 갱신.
	if (Econ->TryPurchase(Row->GrantItemRowId, Row->Price) && Entry)
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

	// 각 엔트리: 잔액으로 살 수 있는지에 따라 구매 버튼 활성/비활성
	for (const TPair<FName, TObjectPtr<ULastFPSShopEntryWidget>>& Pair : EntryByRow)
	{
		if (Pair.Value)
		{
			Pair.Value->SetAffordable(NewCredits >= Pair.Value->GetPrice());
		}
	}
}
