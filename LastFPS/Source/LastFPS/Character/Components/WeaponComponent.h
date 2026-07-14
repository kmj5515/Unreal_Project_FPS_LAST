#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "Utility/LastFPSEnumTypes.h"
#include "WeaponComponent.generated.h"

class ALastFPSProjectile;
class ALastFPSWeaponActor;
class AActor;
class ACharacter;
class AWeaponPickupActor;
class UAnimInstance;
class UGameplayEffect;
class USkeletalMesh;
class USkeletalMeshComponent;
class UParticleSystem;
class USoundBase;
class ULastFPSWeaponDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquippedChanged, bool, bEquipped);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTFPS_API UWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    FTransform GetMuzzleTransform() const;
    bool CanFire() const;
    float GetWeaponBaseDamage() const;
    void PlayFireEffects() const;
	void PlayFireCameraShake() const;
    void ApplyFireAimRecoil(bool bIsAiming);
    void SetWeaponHiddenForAbility(bool bHidden);

    UFUNCTION(BlueprintCallable, Category="Weapon|Reload")
    void DetachMagazineToHand();

    UFUNCTION(BlueprintCallable, Category="Weapon|Reload")
    void RestoreMagazineToWeapon();
    void FireFromClientAim(const FVector& ClientMuzzleLocation, const FVector& ClientCameraLocation, const FVector& ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration);

    UFUNCTION(BlueprintCallable, Category="Weapon|IK")
    bool GetLeftHandIKTransform(USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const;

    UFUNCTION(BlueprintCallable, Category="Weapon|IK")
    bool GetLeftHandIKTransformForTarget(FName TargetName, USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const;

    // 런타임 무기 장착 (서버에서 호출 → Multicast로 전체 적용)
    void EquipWeapon(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass = nullptr);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void EquipWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void UnequipWeapon();

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.0", Units="cm"))
    float AimTraceRange = 10000.f;

    /** 클라이언트 카메라 위치와 서버 시점 위치 사이에 허용할 최대 오차이다. */
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Network", meta=(ClampMin="0.0", Units="cm"))
    float MaxClientCameraLocationError = 200.f;

    /** 클라이언트 총구 위치와 서버 무기 총구 사이에 허용할 최대 오차이다. */
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Network", meta=(ClampMin="0.0", Units="cm"))
    float MaxClientMuzzleLocationError = 150.f;

    /** 네트워크 지터로 정상 발사가 거절되지 않도록 서버 발사 간격에 적용하는 허용 오차이다. */
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Network", meta=(ClampMin="0.0", Units="s"))
    float ServerFireIntervalTolerance = 0.02f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
    FLastFPSDamageRange DamageRange;

    // ── 애니메이션 레이어 ──────────────────────────────────────────
    // 에디터에서 무기별 Layer ABP 클래스 할당 (ABP_Rifle_Layers 등)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Weapon|Animation")
    TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
    TSubclassOf<UAnimInstance> UnarmedAnimLayerClass;

    // 발사 사운드/머즐 플래시는 WeaponComponent가 아니라 WeaponDefinition에서만 관리한다.

    UFUNCTION(BlueprintCallable, Category="Weapon|Debug")
    void TestEquipWeapon();

    UFUNCTION(Server, Reliable)
    void Server_TestEquipWeapon();

    UFUNCTION(Server, Reliable)
    void Server_UnequipWeapon();

    UFUNCTION(Server, Reliable)
    void Server_FireFromClientAim(FVector_NetQuantize ClientMuzzleLocation, FVector_NetQuantize ClientCameraLocation, FVector_NetQuantizeNormal ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration);

    // 에디터에서 테스트할 픽업 BP 클래스 지정
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug")
    TSubclassOf<AWeaponPickupActor> TestPickupClass;
    
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(ReplicatedUsing=OnRep_CurrentWeapon)
    TObjectPtr<ALastFPSWeaponActor> CurrentWeapon;

    UFUNCTION()
    void OnRep_CurrentWeapon();

    UFUNCTION()
    void OnRep_WeaponType();

    UFUNCTION()
    void OnRep_WeaponDefinition();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_DetachMagazineToHand();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_RestoreMagazineToWeapon();

    void ApplyEquip(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass);
    void ApplyWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition);
    void ApplyWeaponDefinitionValues(const ULastFPSWeaponDefinition* NewDefinition);
    void ApplyAnimLayerClass(TSubclassOf<UAnimInstance> AnimLayerClass) const;
    TSubclassOf<UAnimInstance> ResolveCurrentAnimLayerClass() const;

    void AttachWeaponToOwner(ALastFPSWeaponActor* WeaponActor);
    void ApplyWeaponVisibilityOverride();
    ALastFPSWeaponActor* SpawnWeaponActor(USkeletalMesh* NewMesh, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass, ULastFPSWeaponDefinition* Definition = nullptr);
    void DestroyCurrentWeapon();
    void HandleFireFromClientAim(const FVector& ClientMuzzleLocation, const FVector& ClientCameraLocation, const FVector& ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration);
    bool ValidateClientMuzzleLocation(const FVector& ClientMuzzleLocation) const;
    FVector ResolveValidatedTraceStart(const ACharacter& Character, const FVector& ClientCameraLocation) const;
    bool TryConsumeServerFirePermission();
    void ResetPendingAimRecoil();
    void ResetAimRecoilSequence();
    void ApplyDetachMagazineVisual();
    void ApplyRestoreMagazineVisual();

    UPROPERTY(Transient)
    TObjectPtr<AActor> DetachedMagazineVisual;

    UPROPERTY(Transient)
    TObjectPtr<ALastFPSWeaponActor> MagazineSourceWeapon;

    FName HiddenMagazineBoneName;

    int32 WeaponHiddenOverrideCount = 0;
    float PendingAimRecoilPitch = 0.f;
    float PendingAimRecoilYaw = 0.f;
    float RecoverableAimRecoilPitch = 0.f;
    float RecoverableAimRecoilYaw = 0.f;
    double LastAimRecoilTimeSeconds = 0.0;
    double NextAllowedServerFireTimeSeconds = 0.0;
    bool bHasFiredAimRecoil = false;
};
