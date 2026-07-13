#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSProjectileImpactTypes.generated.h"

class AActor;
class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSProjectileImpactContext
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TObjectPtr<AActor> ProjectileActor;

	UPROPERTY()
	TObjectPtr<AActor> HitActor;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	FHitResult HitResult;

	/** 0이면 각 ImpactRule의 기존 범위를 사용한다. */
	float BaseDamageOverride = 0.f;

	FVector GetImpactLocation() const
	{
		if (HitResult.bBlockingHit || !HitResult.ImpactPoint.IsNearlyZero())
		{
			return HitResult.ImpactPoint;
		}

		return HitActor ? HitActor->GetActorLocation() : FVector::ZeroVector;
	}
};
