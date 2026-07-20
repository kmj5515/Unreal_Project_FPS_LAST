#include "Data/Enemies/LastFPSBossLaserAttackData.h"

#include "Utility/LastFPSTags.h"

ULastFPSBossLaserAttackData::ULastFPSBossLaserAttackData()
{
	BlockingObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	BlockingObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	ChargeGameplayCueTag = LastFPSGameplayTags::GameplayCue_Enemy_Boss_LaserCharge;
	PreviewGameplayCueTag = LastFPSGameplayTags::GameplayCue_Enemy_Boss_LaserPreview;
	BeamGameplayCueTag = LastFPSGameplayTags::GameplayCue_Enemy_Boss_LaserFire;
}
