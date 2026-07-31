#include "Data/Definitions/LastFPSPopupCatalog.h"

#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSPopupCatalog)

const FPrimaryAssetType ULastFPSPopupCatalog::PrimaryAssetType =
	LastFPSPrimaryAssetTypes::PopupCatalog;

FPrimaryAssetId ULastFPSPopupCatalog::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}
