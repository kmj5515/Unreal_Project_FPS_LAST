#include "Character/AI/LastFPSCombatTargetRegistry.h"

#include "GameFramework/Actor.h"

void ULastFPSCombatTargetRegistry::RegisterTarget(AActor& Target, const int32 Priority)
{
	if (FEntry* Existing = Entries.FindByPredicate(
		[&Target](const FEntry& Entry) { return Entry.Target.Get() == &Target; }))
	{
		Existing->Priority = Priority;
		return;
	}

	FEntry Entry;
	Entry.Target = &Target;
	Entry.Priority = Priority;
	Entries.Add(MoveTemp(Entry));
}

void ULastFPSCombatTargetRegistry::UnregisterTarget(AActor& Target)
{
	Entries.RemoveAll(
		[&Target](const FEntry& Entry)
		{
			const AActor* Registered = Entry.Target.Get();
			return Registered == nullptr || Registered == &Target;
		});
}

AActor* ULastFPSCombatTargetRegistry::FindBestTarget(
	const FVector& From,
	const float MaxDistance) const
{
	PruneInvalidEntries();

	AActor* BestTarget = nullptr;
	int32 BestPriority = TNumericLimits<int32>::Lowest();
	float BestDistanceSquared = TNumericLimits<float>::Max();
	const float MaxDistanceSquared = MaxDistance > 0.f ? MaxDistance * MaxDistance : 0.f;

	for (const FEntry& Entry : Entries)
	{
		AActor* Target = Entry.Target.Get();
		if (!IsValid(Target))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(From, Target->GetActorLocation());
		if (MaxDistanceSquared > 0.f && DistanceSquared > MaxDistanceSquared)
		{
			continue;
		}

		// 우선순위가 1차 기준, 거리가 2차 기준이다.
		const bool bBetter = Entry.Priority > BestPriority
			|| (Entry.Priority == BestPriority && DistanceSquared < BestDistanceSquared);
		if (bBetter)
		{
			BestTarget = Target;
			BestPriority = Entry.Priority;
			BestDistanceSquared = DistanceSquared;
		}
	}

	return BestTarget;
}

void ULastFPSCombatTargetRegistry::Deinitialize()
{
	Entries.Reset();
	Super::Deinitialize();
}

void ULastFPSCombatTargetRegistry::PruneInvalidEntries() const
{
	Entries.RemoveAll([](const FEntry& Entry) { return !Entry.Target.IsValid(); });
}
