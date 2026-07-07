#pragma once

#include "CoreMinimal.h"
#include "Animation/LastFPSBaseAnimInstance.h"
#include "Engine/EngineTypes.h"
#include "LastFPSCharacterAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS(Abstract, Blueprintable)
class LASTFPS_API ULastFPSCharacterAnimInstance : public ULastFPSBaseAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category="MM|Locomotion", meta=(BlueprintThreadSafe))
	EMMLocomotionState GetLocomotionState() const { return LocomotionState; }

	UFUNCTION(BlueprintPure, Category="MM|Locomotion", meta=(BlueprintThreadSafe))
	bool HasVelocity() const { return bHasVelocity; }

	UFUNCTION(BlueprintPure, Category="MM|Locomotion", meta=(BlueprintThreadSafe))
	bool HasAcceleration() const { return bHasAcceleration; }

	UFUNCTION(BlueprintPure, Category="MM|Locomotion", meta=(BlueprintThreadSafe))
	bool HasMovementInput() const { return bHasMovementInput; }

	UFUNCTION(BlueprintPure, Category="MM|Locomotion", meta=(BlueprintThreadSafe))
	EMMCardinalDirection GetStartCardinalDirection() const { return StartCardinalDirection; }

protected:
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
	float LocomotionAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	float OrientationWarpingAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	float StartDirection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	EMMCardinalDirection StartCardinalDirection = EMMCardinalDirection::Forward;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	EMMCardinalDirection StopCardinalDirection = EMMCardinalDirection::Forward;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	EMMCardinalDirection LocomotionCardinalDirection = EMMCardinalDirection::Forward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion", meta=(ClampMin="0.0", UIMin="0.0", ClampMax="44.0", UIMax="30.0"))
	float CardinalDirectionHysteresis = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion", meta=(ClampMin="0.0", UIMin="0.0"))
	float CardinalDirectionMinSpeed = 20.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	float Yaw = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion")
	float YawInterpSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion")
	float YawDeadZone = 1.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	FVector CharacterVelocity2D = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	FVector LocalVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	FVector LocomotionDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	float DisplacementSinceLastUpdate = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	float DisplacementSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bWantsToSprint = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bWantsToWalk = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bHasAcceleration = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bHasVelocity = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bHasMovementInput = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bDirectionBaseAligned = true;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bIsStarting = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion")
	float StartingSpeedThreshold = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion", meta=(ClampMin="0.0", UIMin="0.0"))
	float StartRequestHoldTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion", meta=(ClampMin="0.0", UIMin="0.0"))
	float MovementInputDeadZone = 0.01f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion", meta=(ClampMin="0.0", UIMin="0.0"))
	float VelocityNearlyZeroTolerance = 0.00001f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion", meta=(ClampMin="0.0", UIMin="0.0", ClampMax="45.0", UIMax="20.0"))
	float DirectionBaseAlignmentTolerance = 5.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|DistanceMatching")
	bool bShouldStop = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|DistanceMatching")
	float DistanceToStop = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|DistanceMatching")
	float StopOrientationWarpingAngle = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|DistanceMatching")
	float StopPredictionMinSpeed = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|DistanceMatching", meta=(ClampMin="0.0", UIMin="0.0"))
	float StopRequestHoldTime = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category="MM|DistanceMatching")
	bool bHasGroundDistance = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|DistanceMatching")
	float GroundDistance = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|DistanceMatching", meta=(ClampMin="0.0", UIMin="0.0"))
	float GroundDistanceTraceLength = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|DistanceMatching")
	TEnumAsByte<ECollisionChannel> GroundDistanceTraceChannel = ECC_Visibility;

	UPROPERTY(BlueprintReadOnly, Category="MM|Locomotion")
	bool bIsPivoting = false;

	UPROPERTY(EditDefaultsOnly, Category="MM|Locomotion")
	float PivotDotThreshold = -0.5f;

	UPROPERTY(EditDefaultsOnly, Category="MM|Locomotion")
	float PivotMinSpeed = 100.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
	EMMAimMode AimMode = EMMAimMode::Hip;

	UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
	bool bIsInCombat = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|Combat")
	bool bIsDead = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Debug")
	bool bDebugDirectionValues = true;

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	virtual void ResetAnimState();
	virtual void OnOwnerCharacterChanged(ACharacter* PreviousCharacter, ACharacter* NewCharacter);
	virtual void CacheMovementModeFlags();
	virtual void UpdateGameThreadCharacterState(float DeltaSeconds);
	virtual void UpdateThreadSafeCharacterState(float DeltaSeconds);
	virtual void UpdateCombatState();
	void SetCachedDirectionBaseRotation(const FRotator& InRotation);

	bool bCachedIsSprinting = false;
	bool bCachedWantsToSprint = false;
	bool bCachedWantsToWalk = false;
	bool bCachedHasMovementInput = false;
	FVector2D CachedMoveInput = FVector2D::ZeroVector;

private:
	bool RefreshOwnerReferences();
	void CacheThreadSafeInputs();
	void UpdateThreadSafeDisplacement(float DeltaSeconds);
	void UpdateThreadSafeLocomotionState(float DeltaSeconds);
	void UpdateThreadSafeMovementValues();
	bool UpdateThreadSafeDirectionValues(const TCHAR*& OutStartDirectionSource);
	void UpdateThreadSafeCardinalDirections();
	void UpdateThreadSafeOrientationWarping();
	void UpdateThreadSafeStartRequest(float DeltaSeconds, bool bHasStartIntent);
	void UpdateThreadSafeLocomotionMode();
	void UpdateThreadSafeDistanceMatching(float DeltaSeconds);
	void UpdateGroundDistance();
	void UpdateThreadSafeYaw(float DeltaSeconds);
	void UpdateThreadSafeAirState();
	void UpdateThreadSafeStance();
	void UpdateThreadSafePivot();
	bool TryGetMoveInputCardinalDirection(EMMCardinalDirection& OutDirection) const;
	float GetDirectionAngleFromVector(const FVector& DirectionVector) const;
	float GetVelocityDirectionAngle() const;
	float GetStartDirectionAngle() const;
	bool IsUsingSprintLocomotion() const;
	void LogDirectionValues(const TCHAR* StartDirectionSource) const;

	float PreviousActorYaw = 0.f;
	FVector PreviousActorLocation = FVector::ZeroVector;
	bool bHasPreviousActorYaw = false;
	bool bHasPreviousActorLocation = false;
	bool bStartRequestConsumed = false;
	bool bStopRequestConsumed = false;
	float StartRequestRemainingTime = 0.f;
	float StopRequestRemainingTime = 0.f;
	bool bHasLatchedStartCardinalDirection = false;
	EMMCardinalDirection LatchedStartCardinalDirection = EMMCardinalDirection::Forward;
	bool bHasThreadSafeInputs = false;
	bool bCachedIsMovingOnGround = false;
	bool bCachedOrientToMovement = false;
	bool bCachedIsFalling = false;
	bool bCachedIsCrouching = false;
	bool bCachedUseSeparateBrakingFriction = false;
	float CachedGroundFriction = 0.f;
	float CachedBrakingFriction = 0.f;
	float CachedBrakingFrictionFactor = 0.f;
	float CachedMaxBrakingDeceleration = 0.f;
	FVector CachedActorLocation = FVector::ZeroVector;
	FVector CachedActorForwardVector = FVector::ForwardVector;
	FVector CachedVelocity = FVector::ZeroVector;
	FVector CachedAcceleration = FVector::ZeroVector;
	FRotator CachedActorRotation = FRotator::ZeroRotator;
	FRotator CachedDirectionBaseRotation = FRotator::ZeroRotator;
};
