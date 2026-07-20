#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "LastFPSBossLaserAttackData.generated.h"

class UGameplayEffect;

/** 보스 레이저의 단계, 판정, 피해 및 연출 계약을 정의하는 불변 설정 데이터다. */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSBossLaserAttackData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULastFPSBossLaserAttackData();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Aim", meta=(ClampMin="0.0", Units="s"))
	float TrackingDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Aim", meta=(ClampMin="0.01", Units="s"))
	float AimUpdateInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Aim", meta=(Units="cm"))
	float TargetAimHeight = 60.f;

	/** 총구가 타겟을 지나치는 근거리에서는 레이저 공격을 시작하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Activation", meta=(ClampMin="0.0", Units="cm"))
	float MinimumActivationDistance = 500.f;

	/** 보스 정면과 타겟 방향의 최소 내적이다. 0은 전방 반구만 허용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Activation", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float MinimumActivationForwardDot = 0.f;

	/** 근접 시 작은 위치 변화로 좌우 방향이 뒤집히지 않도록 보스 정면과 타겟 방향을 혼합하는 거리다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Aim", meta=(ClampMin="1.0", Units="cm"))
	float MinimumHorizontalAimDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Aim", meta=(ClampMin="0.0", ClampMax="89.0", Units="deg"))
	float MaximumUpwardAimPitch = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Aim", meta=(ClampMin="0.0", ClampMax="89.0", Units="deg"))
	float MaximumDownwardAimPitch = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Aim")
	bool bCancelIfTargetLost = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Aim")
	bool bRequireLineOfSight = true;

	/** 추적 종료 후 레이저 방향이 고정되어 플레이어가 회피할 수 있는 예고 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Lock", meta=(ClampMin="0.0", Units="s"))
	float LockDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Beam", meta=(ClampMin="0.01", Units="s"))
	float BeamDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Beam", meta=(ClampMin="0.01", Units="s"))
	float DamageInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Beam", meta=(ClampMin="1.0", Units="cm"))
	float BeamRange = 3500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Beam", meta=(ClampMin="1.0", Units="cm"))
	float BeamRadius = 35.f;

	/** 활성화하면 발사 중에도 목표를 따라가며, 비활성화하면 예고 종료 시점의 방향을 유지한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Beam")
	bool bTrackDuringBeam = false;

	/** 레이저를 막는 월드 오브젝트 타입이다. Pawn을 제외하면 관통형 다중 대상 레이저가 된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Beam")
	TArray<TEnumAsByte<EObjectTypeQuery>> BlockingObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Muzzle")
	FName MuzzleSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Muzzle", meta=(Units="cm"))
	FVector FallbackMuzzleOffset = FVector(50.f, 0.f, 120.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Damage")
	FLastFPSDamageRange DamageRange;

	/** 피해 Gameplay Effect와 상태 Gameplay Effect를 조합할 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Damage")
	TArray<TSubclassOf<UGameplayEffect>> EffectsOnHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Condition")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Condition")
	FGameplayTagContainer BlockedTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Condition")
	bool bIgnoreFriendlyTargets = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Condition")
	bool bHitMultipleTargets = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Recovery", meta=(ClampMin="0.0", Units="s"))
	float RecoveryDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Gameplay Cue")
	FGameplayTag ChargeGameplayCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Gameplay Cue")
	FGameplayTag PreviewGameplayCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Laser|Gameplay Cue")
	FGameplayTag BeamGameplayCueTag;
};
