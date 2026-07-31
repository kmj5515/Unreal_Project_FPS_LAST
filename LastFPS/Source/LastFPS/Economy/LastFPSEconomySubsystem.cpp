#include "Economy/LastFPSEconomySubsystem.h"

#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Data/Tables/LastFPSShopData.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSEconomy, Log, All);

void ULastFPSEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<ULastFPSGameDataSubsystem>();

	Credits = FMath::Max(0, StartingCredits);

	OwnedItems.Reset();
	for (const TPair<FName, int32>& Seed : StartingOwnedItems)
	{
		if (Seed.Key.IsNone() || Seed.Value <= 0)
		{
			continue;
		}
		// 시드 아이템도 정의 검증 — 오타 시드가 유령으로 들어가지 않도록.
		if (!HasItemDefinition(Seed.Key) && GetItemTable())
		{
			UE_LOG(LogLastFPSEconomy, Error,
				TEXT("StartingOwnedItems 의 '%s' 가 DT_ItemData 에 없음 — 시드 무시."), *Seed.Key.ToString());
			continue;
		}
		OwnedItems.Add(Seed.Key, Seed.Value);
	}

	ValidateReferences();
}

const UDataTable* ULastFPSEconomySubsystem::GetItemTable() const
{
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData = GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	return GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item) : nullptr;
}

bool ULastFPSEconomySubsystem::IsItemTableConfigured() const
{
	return GetItemTable() != nullptr;
}

bool ULastFPSEconomySubsystem::HasItemDefinition(FName ItemRowId) const
{
	if (ItemRowId.IsNone())
	{
		return false;
	}
	const UDataTable* Table = GetItemTable();
	if (!Table)
	{
		return false;
	}
	static const FString Context(TEXT("ULastFPSEconomySubsystem::HasItemDefinition"));
	return Table->FindRow<FLastFPSItemData>(ItemRowId, Context, /*bWarnIfRowMissing=*/false) != nullptr;
}

FText ULastFPSEconomySubsystem::GetItemDisplayName(FName ItemRowId) const
{
	if (const UDataTable* Table = GetItemTable())
	{
		static const FString Context(TEXT("ULastFPSEconomySubsystem::GetItemDisplayName"));
		if (const FLastFPSItemData* Row = Table->FindRow<FLastFPSItemData>(ItemRowId, Context, /*bWarnIfRowMissing=*/false))
		{
			if (!Row->ItemName.IsEmpty())
			{
				return Row->ItemName;
			}
		}
	}
	return FText::FromName(ItemRowId); // 정의 없음/이름 비어있음 → RowId 폴백
}

bool ULastFPSEconomySubsystem::TryGetItemRarity(FName ItemRowId, ELastFPSItemRarity& OutRarity) const
{
	if (const UDataTable* Table = GetItemTable())
	{
		static const FString Context(TEXT("ULastFPSEconomySubsystem::TryGetItemRarity"));
		if (const FLastFPSItemData* Row = Table->FindRow<FLastFPSItemData>(ItemRowId, Context, /*bWarnIfRowMissing=*/false))
		{
			OutRarity = Row->Rarity;
			return true;
		}
	}
	return false;
}

void ULastFPSEconomySubsystem::AddCredits(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	Credits += Amount;
	OnCreditsChanged.Broadcast(Credits);
}

bool ULastFPSEconomySubsystem::TryPurchase(FName GrantItemRowId, int32 Price, int32 Count)
{
	const int32 Qty       = FMath::Max(1, Count);
	const int32 UnitCost  = FMath::Max(0, Price);
	const int32 TotalCost = UnitCost * Qty;

	// 차감 전에 정의 검증 — 깨진 참조에 크레딧만 날아가는 유령 구매 차단.
	if (!GrantItemRowId.IsNone() && GetItemTable() && !HasItemDefinition(GrantItemRowId))
	{
		UE_LOG(LogLastFPSEconomy, Error,
			TEXT("구매 거부: GrantItemRowId '%s' 가 DT_ItemData 에 없음(깨진 상점 참조)."), *GrantItemRowId.ToString());
		return false;
	}

	if (Credits < TotalCost)
	{
		return false;
	}

	Credits -= TotalCost;
	OnCreditsChanged.Broadcast(Credits);

	// 화폐 차감 후 아이템 지급 (AddItem 이 OnInventoryChanged 브로드캐스트)
	if (!GrantItemRowId.IsNone())
	{
		AddItem(GrantItemRowId, Qty);
	}

	return true;
}

void ULastFPSEconomySubsystem::AddItem(FName ItemRowId, int32 Count)
{
	if (ItemRowId.IsNone() || Count <= 0)
	{
		return;
	}

	// 정의 없는 아이템은 지급하지 않는다. GameDataSet 로드 실패 시에는 런타임 진행을 허용한다.
	if (GetItemTable() && !HasItemDefinition(ItemRowId))
	{
		UE_LOG(LogLastFPSEconomy, Error,
			TEXT("지급 거부: '%s' 가 DT_ItemData 에 없음."), *ItemRowId.ToString());
		return;
	}

	OwnedItems.FindOrAdd(ItemRowId) += Count;
	OnInventoryChanged.Broadcast();
}

int32 ULastFPSEconomySubsystem::GetItemCount(FName ItemRowId) const
{
	const int32* Found = OwnedItems.Find(ItemRowId);
	return Found ? *Found : 0;
}

void ULastFPSEconomySubsystem::ValidateReferences() const
{
#if !UE_BUILD_SHIPPING
	const UDataTable* Items = GetItemTable();
	if (!Items)
	{
		UE_LOG(LogLastFPSEconomy, Warning, TEXT("GameDataSet에서 DT_ItemData를 찾지 못해 참조 검증을 건너뜁니다."));
		return;
	}

	int32 Broken = 0;

	// 상점 판매 항목의 GrantItemRowId 가 모두 DT_ItemData 에 존재하는지.
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData = GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	if (const UDataTable* Shop = GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Shop) : nullptr)
	{
		static const FString Ctx(TEXT("ULastFPSEconomySubsystem::ValidateReferences|Shop"));
		Shop->ForeachRow<FLastFPSShopItemData>(Ctx,
			[this, &Broken](const FName& RowName, const FLastFPSShopItemData& Row)
			{
				if (!Row.GrantItemRowId.IsNone() && !HasItemDefinition(Row.GrantItemRowId))
				{
					++Broken;
					UE_LOG(LogLastFPSEconomy, Error,
						TEXT("[Shop] 행 '%s' 의 GrantItemRowId '%s' 가 DT_ItemData 에 없음."),
						*RowName.ToString(), *Row.GrantItemRowId.ToString());
				}
			});
	}

	if (Broken == 0)
	{
		UE_LOG(LogLastFPSEconomy, Log, TEXT("경제 테이블 참조 검증 통과 — 깨진 상점 참조 없음."));
	}
	else
	{
		UE_LOG(LogLastFPSEconomy, Error, TEXT("경제 테이블 참조 검증: 깨진 참조 %d건."), Broken);
	}
#endif
}
