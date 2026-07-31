#include "Data/Definitions/LastFPSBattleDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSBattleDefinition)

const FPrimaryAssetType ULastFPSBattleDefinition::PrimaryAssetType(TEXT("BattleDefinition"));

FPrimaryAssetId ULastFPSBattleDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}
