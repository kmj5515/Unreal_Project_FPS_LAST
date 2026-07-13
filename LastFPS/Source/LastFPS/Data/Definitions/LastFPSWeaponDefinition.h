#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "Utility/LastFPSEnumTypes.h"
#include "LastFPSWeaponDefinition.generated.h"

class ALastFPSProjectile;
class ALastFPSWeaponActor;
class UAnimationAsset;
class UAnimInstance;
class UCameraShakeBase;
class UParticleSystem;
class USkeletalMesh;
class USoundBase;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSWeaponAimRecoilSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil")
    bool bEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0", Units="deg"))
    float Strength = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0"))
    float HorizontalRatio = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0", ClampMax="1.0"))
    float RandomnessRatio = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0"))
    float ADSMultiplier = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.01"))
    float InterpolationSpeed = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0", Units="s"))
    float RecoveryDelay = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0", ClampMax="1.0"))
    float RecoveryRatio = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.01"))
    float RecoveryInterpolationSpeed = 10.f;
};

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSWeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    FName WeaponId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    EMMWeaponType WeaponType = EMMWeaponType::Unarmed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    TSubclassOf<ALastFPSWeaponActor> WeaponActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    TSubclassOf<ALastFPSProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
    TSubclassOf<UAnimInstance> AnimLayerClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
    TObjectPtr<UAnimationAsset> FireAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
    TObjectPtr<UAnimationAsset> ReloadAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation", meta=(ClampMin="0.01"))
    float AnimationPlayRate = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
    FName MuzzleSocketName = TEXT("Projectile_Start");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
    FName AttachSocketName = TEXT("WeaponSocket");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
    FName LeftHandIKSocketName = TEXT("LeftHandIK");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
    FName ReloadLeftHandIKTargetName = TEXT("Clip_Bone");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Firing", meta=(ClampMin="0.01"))
    float FireRate = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Damage")
    FLastFPSDamageRange DamageRange;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects")
    TObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects")
    TObjectPtr<UParticleSystem> MuzzleFlashEffect;

	/** 로컬 소유자가 발사할 때 재생할 무기 전용 카메라 셰이크입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects|Camera Shake")
	TSubclassOf<UCameraShakeBase> FireCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects|Camera Shake", meta=(ClampMin="0.0"))
	float FireCameraShakeScale = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Recoil")
    FLastFPSWeaponAimRecoilSettings AimRecoil;

};
