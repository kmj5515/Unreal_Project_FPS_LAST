#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

class ALastFPSProjectile;
class USkeletalMesh;
class UParticleSystem;
class USoundBase;

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
    // 발사 1회당 증가하는 열량
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Overheat")
    float HeatPerShot = 10.f;

    // 최대 열량 — 도달하면 오버히트
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Overheat")
    float MaxHeat = 100.f;

    // 초당 냉각량 (쏘지 않을 때 항상 감소)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Overheat")
    float CooldownRate = 20.f;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Weapon", meta=(AllowPrivateAccess="true"))
    float CurrentHeat = 0.f;

    // 오버히트 상태: true이면 CurrentHeat가 0이 될 때까지 발사 불가
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Weapon", meta=(AllowPrivateAccess="true"))
    bool bIsOverheated = false;
};
