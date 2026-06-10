#include "UI/LastFPSShopScreenWidget.h"

#include "UI/LastFPSShopEntryWidget.h"
#include "Shop/LastFPSShopData.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"

void ULastFPSShopScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildShopList();
}

void ULastFPSShopScreenWidget::RebuildShopList()
{
	if (!Box_ShopList)
	{
		return;
	}

	Box_ShopList->ClearChildren();

	int32 NumRows = 0;

	if (ShopTable && EntryWidgetClass)
	{
		// DataTable 행 순서대로 엔트리 생성. (정렬/필터/재고는 추후 화폐 시스템에서)
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
				++NumRows;
			});
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(NumRows > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void ULastFPSShopScreenWidget::HandleItemPurchased(FName RowName, ULastFPSShopEntryWidget* Entry)
{
	if (Entry)
	{
		Entry->SetPurchased(true);
	}
}
