#include "Data/Definitions/LastFPSPopupCatalog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSPopupCatalog)

const FPrimaryAssetType ULastFPSPopupCatalog::PrimaryAssetType(
	TEXT("PopupCatalog"));

FPrimaryAssetId ULastFPSPopupCatalog::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}
