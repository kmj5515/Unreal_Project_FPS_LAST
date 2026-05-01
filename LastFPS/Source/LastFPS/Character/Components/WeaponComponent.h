#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

class ALastFPSProjectile;
class USkeletalMesh;
class UParticleSystem;
class USoundBase;

// Current, Max, bIsOverheated
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponHeatChanged, float, Current, float, Max, bool, bIsOverheated);

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

    UFUNCTION(BlueprintCallable, Category="Weapon|Overheat")
    float GetCurrentHeat() const { return CurrentHeat; }

    UFUNCTION(BlueprintCallable, Category="Weapon|Overheat")
    float GetMaxHeat() const { return MaxHeat; }

    UFUNCTION(BlueprintCallable, Category="Weapon|Overheat")
    bool IsOverheated() const { return bIsOverheated; }

    // HUD 등이 구독해 열 변화를 수신
    UPROPERTY(BlueprintAssignable, Category="Weapon|Overheat")
    FOnWeaponHeatChanged OnHeatChanged;

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
};
