#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Utility/LastFPSEnumTypes.h"
#include "LastFPSAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS()
class LASTFPS_API ULastFPSAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUninitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
    // ── Locomotion ──────────────────────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    EMMLocomotionState LocomotionState = EMMLocomotionState::Idle;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    EMMStance Stance = EMMStance::Standing;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    EMMAirState AirState = EMMAirState::Grounded;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    float Speed = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    float Direction = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    float StartDirection = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    EMMCardinalDirection StartCardinalDirection = EMMCardinalDirection::Forward;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    EMMCardinalDirection StopCardinalDirection = EMMCardinalDirection::Forward;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    float Yaw = 0.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion")
    float YawInterpSpeed = 12.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion")
    float YawDeadZone = 1.f;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    bool bIsSprinting = false;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    bool bWantsToSprint = false;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    bool bHasAcceleration = false;

    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    bool bIsStarting = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion")
    float StartingSpeedThreshold = 100.f;

    UPROPERTY(BlueprintReadOnly, Category="MM|DistanceMatching")
    bool bShouldStop = false;

    UPROPERTY(BlueprintReadOnly, Category="MM|DistanceMatching")
    float DistanceToStop = 0.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|DistanceMatching")
    float StopPredictionMinSpeed = 150.f;

    // 진행 방향과 입력 방향이 반대(>90°)일 때 true — Chooser/모션매칭 분기용
    UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
    bool bIsPivoting = false;

    // Pivot 감지 임계치 — VelDir·AccDir < 이 값이면 Pivot으로 판정 (기본 -0.5 = 약 120°)
    UPROPERTY(EditDefaultsOnly, Category="MM|Locomotion")
    float PivotDotThreshold = -0.5f;

    // Pivot 판정 최소 속도 — 너무 느릴 땐 Pivot 무시
    UPROPERTY(EditDefaultsOnly, Category="MM|Locomotion")
    float PivotMinSpeed = 100.f;

    // ── Combat ──────────────────────────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
    EMMAimMode AimMode = EMMAimMode::Hip;

    UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
    EMMWeaponType WeaponType = EMMWeaponType::Unarmed;
    
    UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
    EMMCombatState CombatState = EMMCombatState::Idle;
    
    // Aim Offset 용 (-90 ~ 90)
    UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
    float AimPitch = 0.f;

    // 컨트롤러 Yaw - 캐릭터 Yaw (-180 ~ 180). 즉시 동기 상태에선 ~0
    UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
    float AimYaw = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
    bool bIsFiring = false;

    UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
    bool bIsDead = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Debug")
    bool bDebugAimValues = false;

    UPROPERTY(BlueprintReadOnly, Category="MM|IK")
    FTransform LeftHandIKTransform = FTransform::Identity;

    UPROPERTY(BlueprintReadOnly, Category="MM|IK")
    float LeftHandIKAlpha = 0.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|IK")
    FName RightHandBoneName = TEXT("hand_r");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|IK")
    bool bUseReloadLeftHandIKTarget = true;

private:
    void UpdateLocomotionState();
    void UpdateDistanceMatching();
    void UpdateYaw(float DeltaSeconds);
    void UpdateAirState();
    void UpdateStance();
    void UpdateCombatState();
    void UpdatePivot();
    void UpdateHandIK();
    void DebugAimValues() const;
    static EMMCardinalDirection DirectionToCardinalDirection(float InDirection);

    UFUNCTION()
    void OnWeaponEquipped(bool bEquipped);

    UPROPERTY()
    TObjectPtr<ACharacter> OwnerCharacter;

    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> MovementComponent;

    float PreviousActorYaw = 0.f;
    bool bHasPreviousActorYaw = false;
};
