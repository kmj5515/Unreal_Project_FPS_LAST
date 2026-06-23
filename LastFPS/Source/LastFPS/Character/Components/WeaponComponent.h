#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utility/LastFPSEnumTypes.h"
#include "WeaponComponent.generated.h"

class ALastFPSProjectile;
class ALastFPSWeaponActor;
class AWeaponPickupActor;
class UGameplayEffect;
class USkeletalMesh;
class USkeletalMeshComponent;
class UParticleSystem;
class USoundBase;
class ULastFPSWeaponDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquippedChanged, bool, bEquipped);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTFPS_API UWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    FTransform GetMuzzleTransform() const;
    bool CanFire() const;
    void PlayFireEffects() const;

    UFUNCTION(BlueprintCallable, Category="Weapon|Animation")
    void PlayReloadAnimation() const;
    void FireFromClientAim(const FVector& ClientMuzzleLocation, const FVector& ClientCameraLocation, const FVector& ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration);

    UFUNCTION(BlueprintCallable, Category="Weapon|IK")
    bool GetLeftHandIKTransform(USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const;

    UFUNCTION(BlueprintCallable, Category="Weapon|IK")
    bool GetLeftHandIKTransformForTarget(FName TargetName, USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const;

    // 런타임 무기 장착 (서버에서 호출 → Multicast로 전체 적용)
    void EquipWeapon(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass = nullptr);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void EquipWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition);

    // 무기 장착/해제 시 HUD에 알림
    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FOnWeaponEquippedChanged OnWeaponEquippedChanged;

    UFUNCTION(BlueprintCallable, Category="Weapon")
    bool HasWeapon() const { return CurrentWeapon != nullptr; }

    UFUNCTION(BlueprintCallable, Category="Weapon")
    EMMWeaponType GetWeaponType() const { return HasWeapon() ? WeaponType : EMMWeaponType::Unarmed; }

    // 무기 BP마다 Unarmed / Rifle / Pistol 지정 (Chooser Table 분기 입력)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponType, Category="Weapon")
    EMMWeaponType WeaponType = EMMWeaponType::Unarmed;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponDefinition, Category="Weapon")
    TObjectPtr<ULastFPSWeaponDefinition> WeaponDefinition;

    // ── 에디터 설정 ──────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TObjectPtr<USkeletalMesh> WeaponSkeletalMesh;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TSubclassOf<ALastFPSWeaponActor> WeaponActorClass;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TSubclassOf<ALastFPSProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName MuzzleSocketName = TEXT("Projectile_Start");

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName AttachSocketName = TEXT("WeaponSocket");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|IK")
    FName LeftHandIKSocketName = TEXT("LeftHandIK");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|IK")
    FName ReloadLeftHandIKTargetName = TEXT("Clip_Bone");

    // 최소 연사 간격 (초)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    float FireRate = 0.1f;

    // ── 애니메이션 레이어 ──────────────────────────────────────────
    // 에디터에서 무기별 Layer ABP 클래스 할당 (ABP_Rifle_Layers 등)
    UPROPERTY(EditDefaultsOnly, Replicated, Category="Weapon|Animation")
    TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

    // ── 발사 이펙트 ───────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Effects")
    TObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Effects")
    TObjectPtr<UParticleSystem> MuzzleFlashEffect;

    UFUNCTION(BlueprintCallable, Category="Weapon|Debug")
    void TestEquipWeapon();

    UFUNCTION(Server, Reliable)
    void Server_TestEquipWeapon();

    UFUNCTION(Server, Reliable)
    void Server_FireFromClientAim(FVector_NetQuantize ClientMuzzleLocation, FVector_NetQuantize ClientCameraLocation, FVector_NetQuantizeNormal ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration);

    // 에디터에서 테스트할 픽업 BP 클래스 지정
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug")
    TSubclassOf<AWeaponPickupActor> TestPickupClass;
    
protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(ReplicatedUsing=OnRep_CurrentWeapon)
    TObjectPtr<ALastFPSWeaponActor> CurrentWeapon;

    UFUNCTION()
    void OnRep_CurrentWeapon();

    UFUNCTION()
    void OnRep_WeaponType();

    UFUNCTION()
    void OnRep_WeaponDefinition();

    void ApplyEquip(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass);
    void ApplyWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition);
    void ApplyWeaponDefinitionValues(const ULastFPSWeaponDefinition* NewDefinition);

    void AttachWeaponToOwner(ALastFPSWeaponActor* WeaponActor);
    ALastFPSWeaponActor* SpawnWeaponActor(USkeletalMesh* NewMesh, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass, ULastFPSWeaponDefinition* Definition = nullptr);
    void DestroyCurrentWeapon();
    void HandleFireFromClientAim(const FVector& ClientMuzzleLocation, const FVector& ClientCameraLocation, const FVector& ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration);
    bool ValidateClientMuzzleLocation(const FVector& ClientMuzzleLocation) const;
};
