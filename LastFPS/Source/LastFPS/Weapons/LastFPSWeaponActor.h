#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSWeaponActor.generated.h"

class UAnimInstance;
class UAnimationAsset;
class UParticleSystem;
class ULastFPSWeaponDefinition;
class USkeletalMesh;
class USkeletalMeshComponent;
class USoundBase;

UCLASS()
class LASTFPS_API ALastFPSWeaponActor : public AActor
{
    GENERATED_BODY()

public:
    ALastFPSWeaponActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PostInitializeComponents() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializeWeapon(USkeletalMesh* InMesh, ULastFPSWeaponDefinition* InDefinition = nullptr);

    FTransform GetMuzzleTransform(FName MuzzleSocketName) const;
    bool GetSocketTransformInBoneSpace(FName SocketName, USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const;
    void PlayFireEffects(FName MuzzleSocketName) const;
    void SetWeaponHidden(bool bShouldHide);

    UFUNCTION(BlueprintCallable, Category="Weapon|Animation")
    void PlayFireAnimation();

    UFUNCTION(BlueprintCallable, Category="Weapon|Animation")
    void PlayReloadAnimation();

    USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
    USkeletalMesh* GetDefaultWeaponMesh() const;
    ULastFPSWeaponDefinition* GetDefaultWeaponDefinition() const { return DefaultWeaponDefinition; }

private:
    UFUNCTION()
    void OnRep_WeaponHidden();

    void ApplyWeaponHidden();

    UFUNCTION()
    void OnRep_WeaponMeshAsset();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USkeletalMesh> DefaultWeaponMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(AllowPrivateAccess="true"))
    TObjectPtr<ULastFPSWeaponDefinition> DefaultWeaponDefinition;

    UPROPERTY(Replicated)
    TObjectPtr<ULastFPSWeaponDefinition> WeaponDefinition;

    UPROPERTY(ReplicatedUsing=OnRep_WeaponMeshAsset)
    TObjectPtr<USkeletalMesh> WeaponMeshAsset;

    UPROPERTY(ReplicatedUsing=OnRep_WeaponHidden)
    bool bWeaponHidden = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UAnimationAsset> FireAnimation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UAnimationAsset> ReloadAnimation;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Animation", meta=(ClampMin="0.01"))
    float WeaponAnimationPlayRate = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug")
    bool bDrawProjectileStartDebug = true;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug")
    FName DebugProjectileStartSocketName = TEXT("Projectile_Start");

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug", meta=(ClampMin="1.0"))
    float DebugAxisLength = 35.f;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug", meta=(ClampMin="1.0"))
    float DebugSphereRadius = 5.f;
};
