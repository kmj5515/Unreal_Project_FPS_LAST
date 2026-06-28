#include "AbilitySystem/ProjectileRules/LastFPSChainImpactRule.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

void ULastFPSChainImpactRule::ExecuteImpact(const FLastFPSProjectileImpactContext& Context) const
{
	AActor* InitialTarget = Context.HitActor.Get();
	if (!InitialTarget || Radius <= 0.f || MaxChainDepth <= 0 || MaxTargetsPerChain <= 0)
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>> VisitedActors;
	VisitedActors.Add(Context.SourceActor.Get());
	VisitedActors.Add(Context.ProjectileActor.Get());
	VisitedActors.Add(InitialTarget);

	ExecuteChainStep(Context, InitialTarget, 0, VisitedActors);
}

void ULastFPSChainImpactRule::ExecuteChainStep(
	const FLastFPSProjectileImpactContext& Context,
	AActor* ChainSource,
	int32 Depth,
	TSet<TWeakObjectPtr<AActor>>& VisitedActors) const
{
	const AActor* ProjectileActor = Context.ProjectileActor.Get();
	UWorld* World = ProjectileActor ? ProjectileActor->GetWorld() : nullptr;
	if (!World || !ChainSource || Depth >= MaxChainDepth)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ProjectileChainImpact), false, ProjectileActor);
	QueryParams.AddIgnoredActor(Context.ProjectileActor.Get());
	QueryParams.AddIgnoredActor(Context.SourceActor.Get());
	QueryParams.AddIgnoredActor(ChainSource);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHasOverlaps = World->OverlapMultiByObjectType(
		Overlaps,
		ChainSource->GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		Shape,
		QueryParams);

	if (!bHasOverlaps)
	{
		return;
	}

	int32 AppliedTargets = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || VisitedActors.Contains(TWeakObjectPtr<AActor>(TargetActor)))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent(TargetActor);
		if (!DoesTargetPassTags(TargetASC, RequiredTargetTags, BlockedTargetTags))
		{
			continue;
		}

		ApplyGameplayEffectsToTarget(Context, TargetActor, Effects);
		VisitedActors.Add(TargetActor);
		AppliedTargets++;

		ExecuteChainStep(Context, TargetActor, Depth + 1, VisitedActors);

		if (AppliedTargets >= MaxTargetsPerChain)
		{
			break;
		}
	}
}
