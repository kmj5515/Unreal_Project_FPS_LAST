#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ProjectileRules/LastFPSProjectileImpactRule.h"
#include "Engine/DataAsset.h"
#include "Engine/CollisionProfile.h"
#include "Game/Loading/LastFPSRenderWarmupSource.h"
#include "LastFPSAbilityProjectileData.generated.h"

class ALastFPSProjectile;
class UAnimMontage;
class UGameplayEffect;
class ULastFPSProjectileVisualData;
class USoundBase;

/** 발사체에 부착해 재생할 오디오 설정이다. 반복 여부와 감쇠는 Sound Cue가 소유한다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSProjectileAudioSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Audio")
    TObjectPtr<USoundBase> FlightSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Audio", meta=(ClampMin="0.0"))
    float VolumeMultiplier = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Audio", meta=(ClampMin="0.01"))
    float PitchMultiplier = 1.f;
};

/** 투사체 클래스에 종속되지 않고 충돌 형태와 채널 응답을 구성하는 데이터 계약이다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSProjectileCollisionSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Collision", meta=(ClampMin="0.1", Units="cm"))
    FVector BoxExtent = FVector(2.5f, 1.f, 1.f);

    /** 활성화하면 프로젝트 설정에 등록된 Collision Profile의 채널 응답을 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Collision")
    bool bUseCollisionProfile = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Collision", meta=(EditCondition="bUseCollisionProfile"))
    FCollisionProfileName CollisionProfile;

    /** Pawn을 Overlap으로 처리하는 프로필을 사용할 때 피격 이벤트 생성을 허용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Collision")
    bool bGenerateOverlapEvents = true;
};

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSAbilityProjectileData
    : public UPrimaryDataAsset
    , public ILastFPSRenderWarmupSource
{
    GENERATED_BODY()

public:
    virtual void CreateRenderWarmupActors(
        UWorld& World,
        const FTransform& SpawnTransform,
        TArray<AActor*>& OutActors) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
    TSubclassOf<ALastFPSProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0.0"))
    float ProjectileSpeed = 3000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Aim", meta=(ClampMin="0.0"))
    float AimTraceRange = 10000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
    FName SpawnSocketName = TEXT("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
    FVector SpawnLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Collision")
    FLastFPSProjectileCollisionSettings CollisionSettings;

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category="Projectile|Impact")
    TArray<TObjectPtr<ULastFPSProjectileImpactRule>> ImpactRules;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Effects", meta=(DeprecatedProperty, DeprecationMessage="Use ImpactRules with ULastFPSDirectHitImpactRule instead."))
    TArray<TSubclassOf<UGameplayEffect>> EffectsOnHit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Visual")
    TObjectPtr<ULastFPSProjectileVisualData> VisualData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Audio")
    FLastFPSProjectileAudioSettings AudioSettings;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Animation")
    TObjectPtr<UAnimMontage> CastMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Animation", meta=(ClampMin="0.01"))
    float MontagePlayRate = 1.f;
};
