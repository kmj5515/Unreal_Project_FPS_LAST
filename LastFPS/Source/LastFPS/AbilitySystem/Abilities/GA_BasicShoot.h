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

    /**
     * 스프린트 중 사격했을 때 총을 드는 시간.
     * 이 시간이 없으면 스프린트 자세(총을 내린 상태) 그대로 발사되어
     * 총구가 땅을 향한 채 총알이 나간다. 에임 포즈 블렌드 시간과 맞추면 자연스럽다.
     */
    UPROPERTY(EditDefaultsOnly, Category="Shoot|State", meta=(ClampMin="0.0", Units="s"))
    float SprintToFireDelay = 0.18f;

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
    FTimerHandle RaiseWeaponTimerHandle;
    TWeakObjectPtr<UWeaponComponent> CachedWeapon;
};
