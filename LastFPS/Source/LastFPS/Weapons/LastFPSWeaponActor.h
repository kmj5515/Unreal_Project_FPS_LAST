#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSWeaponActor.generated.h"

class UAnimInstance;
class UParticleSystem;
class USkeletalMesh;
class USkeletalMeshComponent;
class USoundBase;

UCLASS()
class LASTFPS_API ALastFPSWeaponActor : public AActor
{
    GENERATED_BODY()

public:
    ALastFPSWeaponActor();

    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializeWeapon(USkeletalMesh* InMesh, UParticleSystem* InMuzzleFlash, USoundBase* InFireSound);

    FTransform GetMuzzleTransform(FName MuzzleSocketName) const;
    bool GetSocketTransformInBoneSpace(FName SocketName, USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const;
    void PlayFireEffects(FName MuzzleSocketName) const;

    USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

private:
    UFUNCTION()
    void OnRep_WeaponMeshAsset();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(ReplicatedUsing=OnRep_WeaponMeshAsset)
    TObjectPtr<USkeletalMesh> WeaponMeshAsset;

    UPROPERTY(Transient)
    TObjectPtr<UParticleSystem> MuzzleFlashEffect;

    UPROPERTY(Transient)
    TObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug")
    bool bDrawProjectileStartDebug = true;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug")
    FName DebugProjectileStartSocketName = TEXT("Projectile_Start");

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug", meta=(ClampMin="1.0"))
    float DebugAxisLength = 35.f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug", meta=(ClampMin="1.0"))
    float DebugSphereRadius = 5.f;
};
