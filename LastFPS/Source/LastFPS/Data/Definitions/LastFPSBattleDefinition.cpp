#include "Data/Definitions/LastFPSBattleDefinition.h"

#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSBattleDefinition)

const FPrimaryAssetType ULastFPSBattleDefinition::PrimaryAssetType =
	LastFPSPrimaryAssetTypes::BattleDefinition;

FPrimaryAssetId ULastFPSBattleDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}
