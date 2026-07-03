#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

namespace LastFPSProjectileAim
{
	bool GetAimViewPoint(const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation);
	FVector GetAimDirection(const AActor* SourceActor);
	FVector GetAimTarget(UWorld* World, const AActor* SourceActor, const FVector& AimDirection, float TraceRange);
}
