#include "Data/Definitions/LastFPSDestinationContentSet.h"

void ULastFPSDestinationContentSet::CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const
{
    OutPaths.Reserve(OutPaths.Num() + RequiredAssets.Num() + RequiredClasses.Num());

    for (const TSoftObjectPtr<UObject>& Asset : RequiredAssets)
    {
        if (!Asset.IsNull())
        {
            OutPaths.Add(Asset.ToSoftObjectPath());
        }
    }

    for (const TSoftClassPtr<UObject>& Class : RequiredClasses)
    {
        if (!Class.IsNull())
        {
            OutPaths.Add(Class.ToSoftObjectPath());
        }
    }
}
