#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Utility/LastFPSEnumTypes.h"
#include "LastFPSWeaponDefinition.generated.h"

class ALastFPSProjectile;
class ALastFPSWeaponActor;
class UAnimationAsset;
class UAnimInstance;
class UParticleSystem;
class USkeletalMesh;
class USoundBase;

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects")
    TObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects")
    TObjectPtr<UParticleSystem> MuzzleFlashEffect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Overheat", meta=(ClampMin="0.0"))
    float HeatPerShot = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Overheat", meta=(ClampMin="0.0"))
    float MaxHeat = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Overheat", meta=(ClampMin="0.0"))
    float CooldownRate = 20.f;
};
