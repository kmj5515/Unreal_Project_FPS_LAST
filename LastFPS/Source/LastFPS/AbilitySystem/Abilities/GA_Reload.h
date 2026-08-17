#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"
#include "GA_Reload.generated.h"

class UAnimMontage;

UCLASS()
class LASTFPS_API UGA_Reload : public ULastFPSActiveGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Reload();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                  const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                             const FGameplayAbilityActorInfo* ActorInfo,
                             const FGameplayAbilityActivationInfo ActivationInfo,
                             bool bReplicateEndAbility,
                             bool bWasCancelled) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Reload|Animation")
    TObjectPtr<UAnimMontage> ReloadMontage;

    UPROPERTY(EditDefaultsOnly, Category="Reload|Animation", meta=(ClampMin="0.01"))
    float MontagePlayRate = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="Reload|State", meta=(ClampMin="0.0"))
    float DefaultReloadDuration = 1.f;

private:
    void FinishReload();

    FTimerHandle ReloadTimerHandle;
    // 리로드를 시작한 슬롯. 서버 타이머는 어빌리티 종료 후에도 살아남으므로,
    // 그 사이 무기가 바뀌었으면 새 무기의 탄창을 채우지 않도록 확인한다.
    int32 ReloadWeaponSlot = INDEX_NONE;
    bool bReloadCompleted = false;
    // 리로드 UI 시작/종료 알림을 정확히 한 쌍으로 맞추기 위한 플래그다. 시작 알림을 보낸 경우에만 종료 알림을 보낸다.
    bool bReloadUINotified = false;
};
