#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LastFPSAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS()
class LASTFPS_API ULastFPSAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
    // BlendSpace X축: 수평 이동 속도 (0 ~ 700)
    UPROPERTY(BlueprintReadOnly, Category="Movement")
    float Speed = 0.f;

    // BlendSpace Y축: 이동 방향 (-180 ~ 180, 좌우 스트레이프용)
    UPROPERTY(BlueprintReadOnly, Category="Movement")
    float Direction = 0.f;

    // 공중 상태 (점프 State Machine 전환용)
    UPROPERTY(BlueprintReadOnly, Category="Movement")
    bool bIsInAir = false;

    // ADS 상태 (상체 레이어 전환용)
    UPROPERTY(BlueprintReadOnly, Category="Combat")
    bool bIsADS = false;

    UPROPERTY(BlueprintReadOnly, Category="Combat")
    bool bIsDead = false;

private:
    UPROPERTY()
    TObjectPtr<ACharacter> OwnerCharacter;

    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> MovementComponent;
};
