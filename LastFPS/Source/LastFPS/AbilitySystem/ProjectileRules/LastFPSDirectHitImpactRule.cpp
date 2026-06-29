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

	const FVector HitLocation = Context.GetImpactLocation();
	DrawDebugPoint(Context, HitLocation);
	if (const AActor* ProjectileActor = Context.ProjectileActor.Get())
	{
		DrawDebugLine(Context, ProjectileActor->GetActorLocation(), HitLocation);
	}

	ApplyGameplayEffectsToTarget(Context, HitActor, Effects, DamageRange);
}
