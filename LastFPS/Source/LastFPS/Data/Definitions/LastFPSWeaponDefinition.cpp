#include "Data/Definitions/LastFPSWeaponDefinition.h"

#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"

const FPrimaryAssetType ULastFPSWeaponDefinition::PrimaryAssetType =
	LastFPSPrimaryAssetTypes::WeaponDefinition;

FPrimaryAssetId ULastFPSWeaponDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}
