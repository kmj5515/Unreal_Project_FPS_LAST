#include "Game/Streaming/LastFPSStreamingLevelTransitionSettings.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool FLastFPSStreamingLevelTransitionRoute::IsValid(
	FString& OutFailureReason) const
{
	if (RouteId.IsNone())
	{
		OutFailureReason = TEXT("RouteId가 비어 있습니다.");
		return false;
	}

	if (SourceWorld.IsNull())
	{
		OutFailureReason = TEXT("SourceWorld가 지정되지 않았습니다.");
		return false;
	}

	if (TriggerMarkerTag.IsNone() || TriggerRouteTag.IsNone())
	{
		OutFailureReason =
			TEXT("TriggerMarkerTag 또는 TriggerRouteTag가 비어 있습니다.");
		return false;
	}

	if (DestinationLevel.IsNull())
	{
		OutFailureReason = TEXT("DestinationLevel이 지정되지 않았습니다.");
		return false;
	}

	if (SourceWorld.ToSoftObjectPath().GetLongPackageName()
		== DestinationLevel.ToSoftObjectPath().GetLongPackageName())
	{
		OutFailureReason = TEXT("SourceWorld와 DestinationLevel이 같습니다.");
		return false;
	}

	if (!DelayedEnemyDefinition.IsNull()
		&& DelayedEnemySpawnDelay < 0.f)
	{
		OutFailureReason =
			TEXT("DelayedEnemySpawnDelay는 0 이상이어야 합니다.");
		return false;
	}

	if (DelayedEnemyDefinition.IsNull()
		&& DelayedEnemySpawnGameplayCueTag.IsValid())
	{
		OutFailureReason =
			TEXT("DelayedEnemySpawnGameplayCueTag를 사용하려면 DelayedEnemyDefinition이 필요합니다.");
		return false;
	}

	OutFailureReason.Reset();
	return true;
}

const FLastFPSStreamingLevelTransitionRoute*
ULastFPSStreamingLevelTransitionSettings::FindRouteForTrigger(
	const UWorld& World,
	const AActor& TriggerActor) const
{
	for (const FLastFPSStreamingLevelTransitionRoute& Route : Routes)
	{
		if (IsRouteForWorld(Route, World)
			&& TriggerActor.ActorHasTag(Route.TriggerMarkerTag)
			&& TriggerActor.ActorHasTag(Route.TriggerRouteTag))
		{
			return &Route;
		}
	}

	return nullptr;
}

void ULastFPSStreamingLevelTransitionSettings::GetRoutesForWorld(
	const UWorld& World,
	TArray<const FLastFPSStreamingLevelTransitionRoute*>& OutRoutes) const
{
	OutRoutes.Reset();
	for (const FLastFPSStreamingLevelTransitionRoute& Route : Routes)
	{
		if (IsRouteForWorld(Route, World))
		{
			OutRoutes.Add(&Route);
		}
	}
}

bool ULastFPSStreamingLevelTransitionSettings::IsRouteForWorld(
	const FLastFPSStreamingLevelTransitionRoute& Route,
	const UWorld& World)
{
	const FString SourcePackageName =
		Route.SourceWorld.ToSoftObjectPath().GetLongPackageName();
	const FString CurrentPackageName =
		UWorld::RemovePIEPrefix(World.GetOutermost()->GetName());
	return !SourcePackageName.IsEmpty()
		&& SourcePackageName == CurrentPackageName;
}
