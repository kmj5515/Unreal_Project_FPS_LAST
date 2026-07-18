#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSBossBurstAttackData.generated.h"

class ULastFPSAbilityProjectileData;

/** 보스의 추적 조준 후 버스트 발사 패턴을 정의하는 불변 설정 데이터다. */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSBossBurstAttackData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Projectile")
	TObjectPtr<ULastFPSAbilityProjectileData> ProjectileData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Aim", meta=(ClampMin="0.0", Units="s"))
	float TrackingDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Aim", meta=(ClampMin="0.01", Units="s"))
	float AimUpdateInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Aim")
	float TargetAimHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Aim")
	bool bCancelIfTargetLost = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Aim")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Lock", meta=(ClampMin="0.0", Units="s"))
	float LockDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Burst", meta=(ClampMin="1"))
	int32 BurstCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Burst", meta=(ClampMin="0.01", Units="s"))
	float ShotInterval = 0.12f;

	/** 활성화하면 각 발사 직전에 현재 타겟 위치로 조준을 다시 갱신한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Burst")
	bool bTrackDuringBurst = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Recovery", meta=(ClampMin="0.0", Units="s"))
	float RecoveryDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Recovery")
	bool bKeepAimDuringRecovery = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Muzzle", meta=(ClampMin="0.0", Units="cm"))
	float FallbackMuzzleHeight = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Burst|Muzzle")
	bool bUseEquippedWeapon = false;

};
