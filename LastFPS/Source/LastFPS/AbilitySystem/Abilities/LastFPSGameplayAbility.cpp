#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ULastFPSGameplayAbility::ULastFPSGameplayAbility()
{
}

void ULastFPSGameplayAbility::DrawDebug(
	const FGameplayAbilityActorInfo*,
	const FGameplayEventData*) const
{
}

bool ULastFPSGameplayAbility::ShouldDrawDebug() const
{
	return bDrawDebug;
}

float ULastFPSGameplayAbility::GetDebugDrawTime() const
{
	return DebugDrawTime;
}

FColor ULastFPSGameplayAbility::GetDebugColor() const
{
	return DebugColor.ToFColor(true);
}

float ULastFPSGameplayAbility::GetDebugPointSize() const
{
	return DebugPointSize;
}

float ULastFPSGameplayAbility::GetDebugLineThickness() const
{
	return DebugLineThickness;
}

UWorld* ULastFPSGameplayAbility::GetDebugWorld(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		return ActorInfo->AvatarActor->GetWorld();
	}

	return GetWorld();
}

void ULastFPSGameplayAbility::DrawDebugPoint(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FVector& Location) const
{
#if ENABLE_DRAW_DEBUG
	if (!ShouldDrawDebug())
	{
		return;
	}

	UWorld* World = GetDebugWorld(ActorInfo);
	if (!World)
	{
		return;
	}

	::DrawDebugPoint(
		World,
		Location,
		GetDebugPointSize(),
		GetDebugColor(),
		false,
		GetDebugDrawTime());
#endif
}

void ULastFPSGameplayAbility::DrawDebugSphere(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FVector& Location,
	float Radius) const
{
#if ENABLE_DRAW_DEBUG
	if (!ShouldDrawDebug() || Radius <= 0.f)
	{
		return;
	}

	UWorld* World = GetDebugWorld(ActorInfo);
	if (!World)
	{
		return;
	}

	::DrawDebugSphere(
		World,
		Location,
		Radius,
		24,
		GetDebugColor(),
		false,
		GetDebugDrawTime(),
		0,
		GetDebugLineThickness());
#endif
}

void ULastFPSGameplayAbility::DrawDebugLine(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FVector& Start,
	const FVector& End) const
{
#if ENABLE_DRAW_DEBUG
	if (!ShouldDrawDebug())
	{
		return;
	}

	UWorld* World = GetDebugWorld(ActorInfo);
	if (!World)
	{
		return;
	}

	::DrawDebugLine(
		World,
		Start,
		End,
		GetDebugColor(),
		false,
		GetDebugDrawTime(),
		0,
		GetDebugLineThickness());
#endif
}
