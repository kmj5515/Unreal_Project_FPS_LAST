#pragma once

#include "CoreMinimal.h"
#include "Animation/LastFPSCharacterAnimInstance.h"
#include "LastFPSAnimInstance.generated.h"

class ACharacter;

UCLASS()
class LASTFPS_API ULastFPSAnimInstance : public ULastFPSCharacterAnimInstance
{
	GENERATED_BODY()

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

	UFUNCTION()
	void OnWeaponEquipped(bool bEquipped);

	int32 ObservedJumpStartSequence = 0;
	int32 CachedJumpStartSequence = 0;
	bool bHasObservedJumpStartSequence = false;
	float JumpStartRequestRemainingTime = 0.f;
};
