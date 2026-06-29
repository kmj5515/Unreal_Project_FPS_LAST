#include "UI/Loading/LastFPSLoadingScreenSet.h"

const FLastFPSLoadingTip* ULastFPSLoadingScreenSet::PickRandomEntry() const
{
	const int32 Count = Entries.Num();
	if (Count == 0)
	{
		return nullptr;
	}
	if (Count == 1)
	{
		LastPickedIndex = 0;
		return &Entries[0];
	}

	int32 Index;
	do
	{
		Index = FMath::RandRange(0, Count - 1);
	}
	while (Index == LastPickedIndex);

	LastPickedIndex = Index;
	return &Entries[Index];
}
