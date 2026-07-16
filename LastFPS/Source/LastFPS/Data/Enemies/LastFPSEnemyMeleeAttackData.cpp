#include "Data/Enemies/LastFPSEnemyMeleeAttackData.h"

#include "AbilitySystem/Effects/GE_DamageInstant.h"
#include "Utility/LastFPSTags.h"

ULastFPSEnemyMeleeAttackData::ULastFPSEnemyMeleeAttackData()
{
	HitEventTag = LastFPSGameplayTags::Event_Montage_MeleeHit;
	EffectsOnHit.Add(ULastFPSGE_DamageInstant::StaticClass());
}
