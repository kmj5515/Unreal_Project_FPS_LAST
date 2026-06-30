#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "GA_Dash.generated.h"

class ALastFPSHero;
class UAnimMontage;

UENUM(BlueprintType)
enum class ELastFPSDashDirection : uint8
{
    Forward,
    Backward,
    Left,
    Right
};

USTRUCT(BlueprintType)
struct FDashMontageInfo
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dash|Animation")
    TObjectPtr<UAnimMontage> Montage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dash|Movement")
    bool bUseMontageRootMotion = true;
};

UCLASS()
class LASTFPS_API UGA_Dash : public ULastFPSGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Dash();

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
    UPROPERTY(EditDefaultsOnly, Category="Dash|Animation")
    FDashMontageInfo DefaultDashMontage;

    UPROPERTY(EditDefaultsOnly, Category="Dash|Animation")
    FDashMontageInfo DashForwardMontage;

    UPROPERTY(EditDefaultsOnly, Category="Dash|Animation")
    FDashMontageInfo DashBackwardMontage;

    UPROPERTY(EditDefaultsOnly, Category="Dash|Animation")
    FDashMontageInfo DashLeftMontage;

    UPROPERTY(EditDefaultsOnly, Category="Dash|Animation")
    FDashMontageInfo DashRightMontage;

    UPROPERTY(EditDefaultsOnly, Category="Dash|Animation", meta=(ClampMin="0.01"))
    float MontagePlayRate = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="Dash|State", meta=(ClampMin="0.0"))
    float DefaultDashDuration = 0.18f;

    UPROPERTY(EditDefaultsOnly, Category="Dash|Movement", meta=(ClampMin="0.0"))
    float DashStrength = 1200.f;
private:
    void FinishDash();
    void OnDashMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    FVector GetCardinalDashDirection(const ALastFPSHero* Hero, ELastFPSDashDirection& OutDashDirection) const;
    const FDashMontageInfo& GetDashMontageInfoForDirection(ELastFPSDashDirection DashDirection) const;

    FTimerHandle DashTimerHandle;
};
