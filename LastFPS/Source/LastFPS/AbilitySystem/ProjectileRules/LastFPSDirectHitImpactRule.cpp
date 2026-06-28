#include "AbilitySystem/ProjectileRules/LastFPSDirectHitImpactRule.h"

#include "AbilitySystemComponent.h"

void ULastFPSDirectHitImpactRule::ExecuteImpact(const FLastFPSProjectileImpactContext& Context) const
{
	AActor* HitActor = Context.HitActor.Get();
	if (!HitActor || HitActor == Context.SourceActor || HitActor == Context.ProjectileActor)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent(HitActor);
	if (!DoesTargetPassTags(TargetASC, RequiredTargetTags, BlockedTargetTags))
	{
		return;
	}

	ApplyGameplayEffectsToTarget(Context, HitActor, Effects);
}
