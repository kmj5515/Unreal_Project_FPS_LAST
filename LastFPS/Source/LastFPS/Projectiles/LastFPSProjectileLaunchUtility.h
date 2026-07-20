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

	/** 활성화하면 소켓 탐색 대신 지정한 월드 위치에서 생성한다. */
	bool bOverrideSpawnLocation = false;
	FVector SpawnLocationOverride = FVector::ZeroVector;
	/** 명시적 생성 위치가 이미 보정된 경우 ProjectileData의 추가 위치 오프셋을 생략한다. */
	bool bApplyProjectileDataSpawnOffset = true;

	/** 활성화하면 방향과 속도를 분리 계산하지 않고 지정한 초기 속도를 그대로 사용한다. */
	bool bOverrideLaunchVelocity = false;
	FVector LaunchVelocityOverride = FVector::ZeroVector;

	bool bOverrideGravityScale = false;
	float GravityScaleOverride = 0.f;
	/** 0 이하는 투사체 클래스의 기본 수명을 유지한다. */
	float LifeSpanOverride = 0.f;
};

namespace LastFPSProjectileLaunch
{
	/** 서버에서 발사 위치를 해석하고 투사체 생성과 초기화를 한 번에 수행한다. */
	LASTFPS_API ALastFPSProjectile* SpawnProjectile(const FLastFPSProjectileLaunchRequest& Request);
}
