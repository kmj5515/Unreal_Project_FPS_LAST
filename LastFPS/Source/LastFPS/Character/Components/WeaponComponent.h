#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utility/LastFPSEnumTypes.h"
#include "WeaponComponent.generated.h"

class ALastFPSProjectile;
class AWeaponPickupActor;
class USkeletalMesh;
class UParticleSystem;
class USoundBase;

// Current, Max, bIsOverheated
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponHeatChanged, float, Current, float, Max, bool, bIsOverheated);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquippedChanged, bool, bEquipped);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTFPS_API UWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    bool CanFire() const;
    void AddHeat();
    FTransform GetMuzzleTransform() const;
    void PlayFireEffects() const;

    // 런타임 무기 장착 (서버에서 호출 → Multicast로 전체 적용)
    void EquipWeapon(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer);

    UFUNCTION(BlueprintCallable, Category="Weapon|Overheat")
    float GetCurrentHeat() const { return CurrentHeat; }

    UFUNCTION(BlueprintCallable, Category="Weapon|Overheat")
    float GetMaxHeat() const { return MaxHeat; }

    UFUNCTION(BlueprintCallable, Category="Weapon|Overheat")
    bool IsOverheated() const { return bIsOverheated; }

    // HUD 등이 구독해 열 변화를 수신
    UPROPERTY(BlueprintAssignable, Category="Weapon|Overheat")
    FOnWeaponHeatChanged OnHeatChanged;

    // 무기 장착/해제 시 HUD에 알림
    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FOnWeaponEquippedChanged OnWeaponEquippedChanged;

    UFUNCTION(BlueprintCallable, Category="Weapon")
    bool HasWeapon() const { return WeaponSkeletalMesh != nullptr; }

    UFUNCTION(BlueprintCallable, Category="Weapon")
    EMMWeaponType GetWeaponType() const { return HasWeapon() ? WeaponType : EMMWeaponType::Unarmed; }

    // 무기 BP마다 Unarmed / Rifle / Pistol 지정 (Chooser Table 분기 입력)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    EMMWeaponType WeaponType = EMMWeaponType::Unarmed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    // ── 에디터 설정 ──────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TObjectPtr<USkeletalMesh> WeaponSkeletalMesh;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TSubclassOf<ALastFPSProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName MuzzleSocketName = TEXT("MuzzleFlash");

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName AttachSocketName = TEXT("WeaponSocket");

    // 최소 연사 간격 (초)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    float FireRate = 0.1f;

    // ── 애니메이션 레이어 ──────────────────────────────────────────
    // 에디터에서 무기별 Layer ABP 클래스 할당 (ABP_Rifle_Layers 등)
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Animation")
    TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

    // ── 발사 이펙트 ───────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Effects")
    TObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Effects")
    TObjectPtr<UParticleSystem> MuzzleFlashEffect;

    // ── 오버히트 설정 ─────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Overheat")
    float HeatPerShot = 10.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Overheat")
    float MaxHeat = 100.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Overheat")
    float CooldownRate = 20.f;
    
    UFUNCTION(BlueprintCallable, Category="Weapon|Debug")
    void TestEquipWeapon();

    UFUNCTION(Server, Reliable)
    void Server_TestEquipWeapon();

    // 에디터에서 테스트할 픽업 BP 클래스 지정
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug")
    TSubclassOf<AWeaponPickupActor> TestPickupClass;
    
protected:
    virtual void BeginPlay() override;

private:
    // ReplicatedUsing: 클라이언트에서 값이 복제될 때 OnRep_HeatState 호출
    UPROPERTY(ReplicatedUsing=OnRep_HeatState, BlueprintReadOnly, Category="Weapon", meta=(AllowPrivateAccess="true"))
    float CurrentHeat = 0.f;

    UPROPERTY(ReplicatedUsing=OnRep_HeatState, BlueprintReadOnly, Category="Weapon", meta=(AllowPrivateAccess="true"))
    bool bIsOverheated = false;

    UFUNCTION()
    void OnRep_HeatState();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_EquipWeapon(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer);

    void ApplyEquip(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer);
};
