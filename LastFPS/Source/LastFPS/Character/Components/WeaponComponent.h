#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

class ALastFPSProjectile;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTFPS_API UWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    bool CanFire() const;
    void ConsumeAmmo();
    FTransform GetMuzzleTransform() const;

    UFUNCTION(BlueprintCallable, Category="Weapon")
    int32 GetCurrentAmmo() const { return CurrentAmmo; }

    UFUNCTION(BlueprintCallable, Category="Weapon")
    int32 GetMaxAmmo() const { return MaxAmmo; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    // ── 에디터 설정 ──────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TSubclassOf<ALastFPSProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName MuzzleSocketName = TEXT("MuzzleSocket");

    // 캐릭터 스켈레톤에 무기를 붙일 소켓
    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName AttachSocketName = TEXT("hand_r");

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    int32 MaxAmmo = 30;

    // 최소 연사 간격 (초) — GA_BasicShoot에서 쿨다운으로 활용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    float FireRate = 0.1f;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Weapon", meta=(AllowPrivateAccess="true"))
    int32 CurrentAmmo;
};
