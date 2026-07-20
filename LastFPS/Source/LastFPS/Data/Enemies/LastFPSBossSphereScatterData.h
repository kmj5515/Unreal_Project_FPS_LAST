#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "LastFPSBossSphereScatterData.generated.h"

class ULastFPSAbilityProjectileData;

/** 배열 원소별 생성 오프셋에 거리 단위를 보존하는 데이터 계약이다. */
USTRUCT(BlueprintType)
struct FLastFPSSphereScatterSpawnOffset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Spawn", meta=(Units="cm"))
	FVector Offset = FVector::ZeroVector;
};

/** 보스 주변 착지 범위와 구체의 포물선 생성 규칙을 정의하는 불변 설정 데이터다. */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSBossSphereScatterData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Projectile")
	TObjectPtr<ULastFPSAbilityProjectileData> ProjectileData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Spawn", meta=(ClampMin="1"))
	int32 ProjectileCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Spawn", meta=(ClampMin="0.0", Units="s"))
	float SpawnInterval = 0.08f;

	/** 지정하면 보스 루트 대신 해당 소켓의 트랜스폼을 생성 기준으로 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Spawn")
	FName SpawnSocketName = NAME_None;

	/** 생성 기준 트랜스폼의 로컬 공간에서 모든 구체에 공통 적용하는 오프셋이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Spawn", meta=(Units="cm"))
	FVector SpawnOriginOffset = FVector(0.f, 0.f, 250.f);

	/** 인덱스별 로컬 오프셋이다. 배열보다 구체가 많으면 나머지는 공통 오프셋만 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Spawn", meta=(TitleProperty="Offset"))
	TArray<FLastFPSSphereScatterSpawnOffset> ProjectileSpawnOffsets;

	/** 동일 지점 생성으로 구체끼리 충돌하는 것을 줄이는 수평 분리 반경이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Spawn", meta=(ClampMin="0.0", Units="cm"))
	float SpawnSeparationRadius = 30.f;

	/** 보스 로컬 공간에서 착지 분산 원의 중심을 이동한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Landing", meta=(Units="cm"))
	FVector LandingCenterOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Landing", meta=(ClampMin="0.0", Units="cm"))
	float MinimumLandingRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Landing", meta=(ClampMin="1.0", Units="cm"))
	float MaximumLandingRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Landing")
	bool bProjectLandingToGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Landing")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Landing", meta=(ClampMin="0.0", Units="cm"))
	float GroundTraceStartOffset = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Landing", meta=(ClampMin="1.0", Units="cm"))
	float GroundTraceDistance = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Landing", meta=(Units="cm"))
	float GroundSurfaceOffset = 5.f;

	/** 0에 가까울수록 높은 포물선, 1에 가까울수록 낮은 포물선을 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Ballistics", meta=(ClampMin="0.05", ClampMax="0.95"))
	float ArcParam = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Ballistics", meta=(ClampMin="0.01"))
	float ProjectileGravityScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Ballistics", meta=(ClampMin="0.1", Units="s"))
	float ProjectileLifeSpan = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sphere Scatter|Recovery", meta=(ClampMin="0.0", Units="s"))
	float RecoveryDuration = 0.5f;
};
