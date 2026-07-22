#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"
#include "GA_BasicShoot.generated.h"

class UWeaponComponent;
class ACharacter;
class ALastFPSHero;
class UGameplayEffect;
class UAnimMontage;

UCLASS()
class LASTFPS_API UGA_BasicShoot : public ULastFPSActiveGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_BasicShoot();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                  const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                             const FGameplayAbilityActorInfo* ActorInfo,
                             const FGameplayAbilityActivationInfo ActivationInfo,
                             bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    // true = 연사, false = 단발
    UPROPERTY(EditDefaultsOnly, Category="Shoot")
    bool bIsAutoFire = false;

    // 피격 시 적용할 데미지 GE (에디터에서 할당)
    UPROPERTY(EditDefaultsOnly, Category="Shoot")
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditDefaultsOnly, Category="Shoot|Animation")
    TObjectPtr<UAnimMontage> HipFireMontage;

    UPROPERTY(EditDefaultsOnly, Category="Shoot|Animation")
    TObjectPtr<UAnimMontage> ADSFireMontage;

    UPROPERTY(EditDefaultsOnly, Category="Shoot|Animation", meta=(ClampMin="0.0"))
    float CancelMontageBlendOutTime = 0.05f;

    UPROPERTY(EditDefaultsOnly, Category="Shoot|State", meta=(ClampMin="0.0"))
    float MinAttackStateDuration = 0.12f;

private:
    void Fire();
    void FinishAbility();
    void TryStartAutoReload(ALastFPSHero* Hero, UWeaponComponent* Weapon) const;

    // 로컬 클라이언트: 사운드 + 머즐플래시 즉시 재생 (클라이언트 예측)
    void LocalFire(UWeaponComponent* Weapon);
    void StopFireMontage() const;

    // 서버 전용: LineTrace 히트 판정 + 데미지 GE 적용 + VFX 투사체 스폰
    UWeaponComponent* GetWeaponComponent() const;

    FTimerHandle FireTimerHandle;
    FTimerHandle FinishAbilityTimerHandle;
    TWeakObjectPtr<UWeaponComponent> CachedWeapon;
};
