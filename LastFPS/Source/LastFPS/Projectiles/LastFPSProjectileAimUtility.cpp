#include "Projectiles/LastFPSProjectileAimUtility.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

bool LastFPSProjectileAim::GetAimViewPoint(const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation)
{
	if (!SourceActor)
	{
		return false;
	}

	if (const APawn* Pawn = Cast<APawn>(SourceActor))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			Controller->GetPlayerViewPoint(OutLocation, OutRotation);
			return true;
		}
	}

	OutLocation = SourceActor->GetActorLocation();
	OutRotation = SourceActor->GetActorRotation();
	return true;
}

FVector LastFPSProjectileAim::GetAimDirection(const AActor* SourceActor)
{
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetAimViewPoint(SourceActor, ViewLocation, ViewRotation))
	{
		return FVector::ForwardVector;
	}

	return ViewRotation.Vector().GetSafeNormal();
}

FVector LastFPSProjectileAim::GetAimTarget(
	UWorld* World,
	const AActor* SourceActor,
	const FVector& AimDirection,
	const float TraceRange)
{
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetAimViewPoint(SourceActor, ViewLocation, ViewRotation))
	{
		return FVector::ZeroVector;
	}

	const FVector TraceDirection = AimDirection.IsNearlyZero()
		? ViewRotation.Vector().GetSafeNormal()
		: AimDirection.GetSafeNormal();
	const FVector TraceEnd = ViewLocation + TraceDirection * TraceRange;

	if (!World || !SourceActor)
	{
		return TraceEnd;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ProjectileAimTrace), false, SourceActor);
	QueryParams.AddIgnoredActor(SourceActor);

	TArray<AActor*> AttachedActors;
	SourceActor->GetAttachedActors(AttachedActors);
	QueryParams.AddIgnoredActors(AttachedActors);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByObjectType(
		HitResult,
		ViewLocation,
		TraceEnd,
		ObjectParams,
		QueryParams);

	return bHit ? HitResult.ImpactPoint : TraceEnd;
}
