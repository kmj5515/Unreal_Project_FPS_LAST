#pragma once

#include "CoreMinimal.h"
#include "Animation/LastFPSCharacterAnimInstance.h"
#include "Character/Animation/LastFPSGrapplingAnimationTypes.h"
#include "LastFPSAnimInstance.generated.h"

class ACharacter;

UCLASS()
class LASTFPS_API ULastFPSAnimInstance : public ULastFPSCharacterAnimInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="MM|IK|Grappling", meta=(BlueprintThreadSafe))
	FVector GetGrapplingIKEffectorLocation() const { return GrapplingIKEffectorLocation; }

	UFUNCTION(BlueprintPure, Category="MM|IK|Grappling", meta=(BlueprintThreadSafe))
	FVector GetGrapplingIKJointTargetLocation() const { return GrapplingIKJointTargetLocation; }

	UFUNCTION(BlueprintPure, Category="MM|IK|Grappling", meta=(BlueprintThreadSafe))
	FRotator GetGrapplingIKHandRotation() const { return GrapplingIKHandRotation; }

	UFUNCTION(BlueprintPure, Category="MM|IK|Grappling", meta=(BlueprintThreadSafe))
	float GetGrapplingIKAlpha() const { return GrapplingIKAlpha; }

	UFUNCTION(BlueprintPure, Category="MM|IK|Grappling", meta=(BlueprintThreadSafe))
	float GetGrapplingBodyPitch() const { return GrapplingBodyPitch; }

	UFUNCTION(BlueprintPure, Category="MM|IK|Grappling", meta=(BlueprintThreadSafe))
	ELastFPSGrapplingAnimationPhase GetGrapplingAnimationPhase() const { return GrapplingAnimationPhase; }

	UFUNCTION(BlueprintPure, Category="MM|IK|Grappling", meta=(BlueprintThreadSafe))
	float GetGrapplingHookFlightDuration() const { return GrapplingHookFlightDuration; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bJumpStartRequested = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bJumpApexRequested = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	int32 JumpStartSequence = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion", meta=(ClampMin="0.0", UIMin="0.0"))
	float JumpStartRequestHoldTime = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
	EMMWeaponType WeaponType = EMMWeaponType::Unarmed;

	UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
	EMMCombatState CombatState = EMMCombatState::Idle;

	UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
	bool bIsFiring = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
	bool bIsCasting = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|IK")
	FTransform LeftHandIKTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category="MM|IK")
	float LeftHandIKAlpha = 0.f;

	/** 게임 스레드에서 캐시해 AnimGraph의 병렬 평가가 캐릭터 컴포넌트에 접근하지 않게 한다. */
	UPROPERTY(BlueprintReadOnly, Category="MM|IK|Grappling")
	FVector GrapplingIKEffectorLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="MM|IK|Grappling")
	FVector GrapplingIKJointTargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="MM|IK|Grappling")
	FRotator GrapplingIKHandRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category="MM|IK|Grappling")
	float GrapplingIKAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|IK|Grappling")
	float GrapplingBodyPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|IK|Grappling")
	ELastFPSGrapplingAnimationPhase GrapplingAnimationPhase = ELastFPSGrapplingAnimationPhase::None;

	UPROPERTY(BlueprintReadOnly, Category="MM|IK|Grappling")
	float GrapplingHookFlightDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|IK")
	FName RightHandBoneName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|IK")
	bool bUseReloadLeftHandIKTarget = true;

	virtual void ResetAnimState() override;
	virtual void OnOwnerCharacterChanged(ACharacter* PreviousCharacter, ACharacter* NewCharacter) override;
	virtual void CacheMovementModeFlags() override;
	virtual void UpdateGameThreadCharacterState(float DeltaSeconds) override;
	virtual void UpdateThreadSafeCharacterState(float DeltaSeconds) override;
	virtual void UpdateCombatState() override;

private:
	void UpdateJumpStartRequest(float DeltaSeconds);
	void UpdateHandIK();
	void UpdateGrapplingIK();

	UFUNCTION()
	void OnWeaponEquipped(bool bEquipped);

	int32 ObservedJumpStartSequence = 0;
	int32 CachedJumpStartSequence = 0;
	bool bHasObservedJumpStartSequence = false;
	float JumpStartRequestRemainingTime = 0.f;
};
