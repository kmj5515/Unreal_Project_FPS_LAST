#include "Economy/LastFPSEconomySubsystem.h"

void ULastFPSEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Credits = FMath::Max(0, StartingCredits);

	OwnedItems.Reset();
	for (const TPair<FName, int32>& Seed : StartingOwnedItems)
	{
		if (!Seed.Key.IsNone() && Seed.Value > 0)
		{
			OwnedItems.Add(Seed.Key, Seed.Value);
		}
	}
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

bool ULastFPSEconomySubsystem::TryPurchase(FName GrantItemRowId, int32 Price)
{
	const int32 Cost = FMath::Max(0, Price);
	if (Credits < Cost)
	{
		return false;
	}

	Credits -= Cost;
	OnCreditsChanged.Broadcast(Credits);

	// 화폐 차감 후 아이템 지급 (AddItem 이 OnInventoryChanged 브로드캐스트)
	if (!GrantItemRowId.IsNone())
	{
		AddItem(GrantItemRowId, 1);
	}

	return true;
}

void ULastFPSEconomySubsystem::AddItem(FName ItemRowId, int32 Count)
{
	if (ItemRowId.IsNone() || Count <= 0)
	{
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
