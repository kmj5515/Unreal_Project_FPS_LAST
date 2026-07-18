#include "Data/Enemies/LastFPSBossGroundSlamData.h"

#include "Utility/LastFPSTags.h"

ULastFPSBossGroundSlamData::ULastFPSBossGroundSlamData()
{
	AttackActorClass = ALastFPSExpandingMeshAttackActor::StaticClass();
	ImpactEventTag = LastFPSGameplayTags::Event_Montage_GroundSlamImpact;
}
