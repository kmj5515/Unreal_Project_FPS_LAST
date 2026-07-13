#pragma once

#include "CoreMinimal.h"

class AActor;
class ALastFPSProjectile;
class ULastFPSAbilityProjectileData;

struct FLastFPSProjectileLaunchRequest
{
	/** 호출 동안만 유효하며 소유권을 전달하지 않는다. */
	AActor* SourceActor = nullptr;
	const ULastFPSAbilityProjectileData* ProjectileData = nullptr;
	FVector AimTarget = FVector::ZeroVector;
	FVector FallbackAimDirection = FVector::ForwardVector;
	float FallbackMuzzleHeight = 60.f;
	/** 0이면 ImpactRule의 기존 데미지 범위를 사용한다. */
	float BaseDamageOverride = 0.f;
	bool bUseEquippedWeapon = false;
};

namespace LastFPSProjectileLaunch
{
	/** 서버에서 발사 위치를 해석하고 투사체 생성과 초기화를 한 번에 수행한다. */
	LASTFPS_API ALastFPSProjectile* SpawnProjectile(const FLastFPSProjectileLaunchRequest& Request);
}
