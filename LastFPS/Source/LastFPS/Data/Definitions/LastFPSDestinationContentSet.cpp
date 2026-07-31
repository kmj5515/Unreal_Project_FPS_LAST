#include "Data/Definitions/LastFPSDestinationContentSet.h"

#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"

const FPrimaryAssetType ULastFPSDestinationContentSet::PrimaryAssetType =
    LastFPSPrimaryAssetTypes::DestinationContentSet;

FPrimaryAssetId ULastFPSDestinationContentSet::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

void ULastFPSDestinationContentSet::CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const
{
    OutPaths.Reserve(OutPaths.Num() + RequiredAssets.Num() + RequiredClasses.Num());

    for (const TSoftObjectPtr<UObject>& Asset : RequiredAssets)
    {
        if (!Asset.IsNull())
        {
            OutPaths.AddUnique(Asset.ToSoftObjectPath());
        }
    }

    for (const TSoftClassPtr<UObject>& Class : RequiredClasses)
    {
        if (!Class.IsNull())
        {
            OutPaths.AddUnique(Class.ToSoftObjectPath());
        }
    }

    for (const ULastFPSDestinationFeature* Feature : Features)
    {
        if (Feature)
        {
            Feature->CollectRequiredPaths(OutPaths);
        }
    }
}
