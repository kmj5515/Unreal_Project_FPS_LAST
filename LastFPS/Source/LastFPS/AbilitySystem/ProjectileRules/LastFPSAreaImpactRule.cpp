#include "AbilitySystem/ProjectileRules/LastFPSAreaImpactRule.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"

void ULastFPSAreaImpactRule::ExecuteImpact(const FLastFPSProjectileImpactContext& Context) const
{
	const AActor* ProjectileActor = Context.ProjectileActor.Get();
	UWorld* World = ProjectileActor ? ProjectileActor->GetWorld() : nullptr;
	if (!World || Radius <= 0.f || MaxTargets <= 0)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ProjectileAreaImpact), false, ProjectileActor);
	QueryParams.AddIgnoredActor(Context.ProjectileActor.Get());
	QueryParams.AddIgnoredActor(Context.SourceActor.Get());
	if (!bIncludeDirectHitTarget)
	{
		QueryParams.AddIgnoredActor(Context.HitActor.Get());
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHasOverlaps = World->OverlapMultiByObjectType(
		Overlaps,
		Context.GetImpactLocation(),
		FQuat::Identity,
		ObjectParams,
		Shape,
		QueryParams);

	if (!bHasOverlaps)
	{
		return;
	}

	int32 AppliedTargets = 0;
	TSet<TWeakObjectPtr<AActor>> AppliedActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || AppliedActors.Contains(TWeakObjectPtr<AActor>(TargetActor)))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent(TargetActor);
		if (!DoesTargetPassTags(TargetASC, RequiredTargetTags, BlockedTargetTags))
		{
			continue;
		}

		ApplyGameplayEffectsToTarget(Context, TargetActor, Effects);
		AppliedActors.Add(TargetActor);
		AppliedTargets++;

		if (AppliedTargets >= MaxTargets)
		{
			break;
		}
	}
}
