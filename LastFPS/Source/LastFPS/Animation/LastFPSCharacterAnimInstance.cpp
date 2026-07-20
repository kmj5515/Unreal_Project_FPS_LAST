#include "Animation/LastFPSCharacterAnimInstance.h"

#include "Character/LastFPSCharacterBase.h"
#include "Character/Components/WeaponComponent.h"
#include "Character/Interfaces/LastFPSWeaponUser.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULastFPSCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	RefreshOwnerReferences();
}

void ULastFPSCharacterAnimInstance::NativeUninitializeAnimation()
{
	OnOwnerCharacterChanged(OwnerCharacter, nullptr);
	OwnerCharacter = nullptr;
	MovementComponent = nullptr;
	ResetAnimState();

	Super::NativeUninitializeAnimation();
}

void ULastFPSCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!RefreshOwnerReferences())
	{
		return;
	}

	UpdateGroundDistance();
	UpdateCombatState();
	UpdateWeaponHandIK();
	UpdateGameThreadCharacterState(DeltaSeconds);
	CacheThreadSafeInputs();
}

void ULastFPSCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!bHasThreadSafeInputs)
	{
		return;
	}

	UpdateThreadSafeDisplacement(DeltaSeconds);
	UpdateThreadSafeYaw(DeltaSeconds);
	UpdateThreadSafeLocomotionState(DeltaSeconds);
	UpdateThreadSafeDistanceMatching(DeltaSeconds);
	UpdateThreadSafeAirState();
	UpdateThreadSafeStance();
	UpdateThreadSafePivot();
	UpdateThreadSafeCharacterState(DeltaSeconds);
}

void ULastFPSCharacterAnimInstance::ResetAnimState()
{
	LocomotionState = EMMLocomotionState::Idle;
	Stance = EMMStance::Standing;
	AirState = EMMAirState::Grounded;
	Speed = 0.f;
	Direction = 0.f;
	LocomotionAngle = 0.f;
	OrientationWarpingAngle = 0.f;
	StartDirection = 0.f;
	StartCardinalDirection = EMMCardinalDirection::Forward;
	StopCardinalDirection = EMMCardinalDirection::Forward;
	LocomotionCardinalDirection = EMMCardinalDirection::Forward;
	Yaw = 0.f;
	Velocity = FVector::ZeroVector;
	CharacterVelocity2D = FVector::ZeroVector;
	LocalVelocity = FVector::ZeroVector;
	LocomotionDirection = FVector::ForwardVector;
	DisplacementSinceLastUpdate = 0.f;
	DisplacementSpeed = 0.f;
	bHasPreviousActorYaw = false;
	bHasPreviousActorLocation = false;
	bStartRequestConsumed = false;
	bStopRequestConsumed = false;
	StartRequestRemainingTime = 0.f;
	StopRequestRemainingTime = 0.f;
	bHasLatchedStartCardinalDirection = false;
	LatchedStartCardinalDirection = EMMCardinalDirection::Forward;
	bIsSprinting = false;
	bWantsToSprint = false;
	bWantsToWalk = false;
	bHasAcceleration = false;
	bHasVelocity = false;
	bHasMovementInput = false;
	bDirectionBaseAligned = true;
	bIsStarting = false;
	bShouldStop = false;
	DistanceToStop = 0.f;
	StopOrientationWarpingAngle = 0.f;
	bHasGroundDistance = false;
	GroundDistance = 0.f;
	bIsPivoting = false;
	AimMode = EMMAimMode::Hip;
	bIsInCombat = false;
	AimPitch = 0.f;
	AimYaw = 0.f;
	bIsDead = false;
	bHasThreadSafeInputs = false;
	WeaponLeftHandIKTransform = FTransform::Identity;
	WeaponLeftHandIKAlpha = 0.f;
	WeaponLeftHandIKJointTargetLocation = FVector::ZeroVector;
	bCachedIsSprinting = false;
	bCachedWantsToSprint = false;
	bCachedWantsToWalk = false;
	bCachedHasMovementInput = false;
	CachedMoveInput = FVector2D::ZeroVector;
	bCachedIsMovingOnGround = false;
	bCachedHasMovementComponent = false;
	bCachedOrientToMovement = false;
	bCachedIsFalling = false;
	bCachedIsCrouching = false;
	bCachedUseSeparateBrakingFriction = false;
	CachedGroundFriction = 0.f;
	CachedBrakingFriction = 0.f;
	CachedBrakingFrictionFactor = 0.f;
	CachedMaxBrakingDeceleration = 0.f;
	CachedBrakingDecelerationWalking = 0.f;
	CachedMaxWalkSpeed = 0.f;
	CachedGravityZ = 0.f;
	CachedActorLocation = FVector::ZeroVector;
	CachedActorForwardVector = FVector::ForwardVector;
	CachedVelocity = FVector::ZeroVector;
	CachedAcceleration = FVector::ZeroVector;
	CachedActorRotation = FRotator::ZeroRotator;
	CachedDirectionBaseRotation = FRotator::ZeroRotator;
}

void ULastFPSCharacterAnimInstance::OnOwnerCharacterChanged(ACharacter*, ACharacter*)
{
}

void ULastFPSCharacterAnimInstance::CacheMovementModeFlags()
{
}

void ULastFPSCharacterAnimInstance::SetCachedDirectionBaseRotation(const FRotator& InRotation)
{
	CachedDirectionBaseRotation = InRotation;
}

void ULastFPSCharacterAnimInstance::UpdateGameThreadCharacterState(float)
{
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeCharacterState(float)
{
}

void ULastFPSCharacterAnimInstance::UpdateWeaponHandIK()
{
	WeaponLeftHandIKTransform = FTransform::Identity;
	WeaponLeftHandIKAlpha = 0.f;
	WeaponLeftHandIKJointTargetLocation = FVector::ZeroVector;

	if (!OwnerCharacter || !OwnerCharacter->GetMesh())
	{
		return;
	}

	const ILastFPSWeaponUser* WeaponUser = Cast<ILastFPSWeaponUser>(OwnerCharacter);
	UWeaponComponent* Weapon = WeaponUser ? WeaponUser->GetWeaponComponent() : nullptr;
	if (!Weapon || !Weapon->HasWeapon())
	{
		return;
	}

	FTransform IKTransform;
	if (Weapon->GetLeftHandIKTransform(
		OwnerCharacter->GetMesh(),
		WeaponIKRightHandBoneName,
		IKTransform))
	{
		WeaponLeftHandIKTransform = IKTransform;
		WeaponLeftHandIKAlpha = 1.f;
		Weapon->GetLeftHandIKJointTargetLocation(
			OwnerCharacter->GetMesh(),
			WeaponLeftHandIKJointTargetLocation);
	}
}

bool ULastFPSCharacterAnimInstance::RefreshOwnerReferences()
{
	ACharacter* CurrentCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (!CurrentCharacter)
	{
		CurrentCharacter = Cast<ACharacter>(GetOwningActor());
	}

	if (OwnerCharacter == CurrentCharacter && MovementComponent)
	{
		return true;
	}

	ACharacter* PreviousCharacter = OwnerCharacter;
	OwnerCharacter = CurrentCharacter;
	MovementComponent = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
	PreviousActorLocation = OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
	ResetAnimState();
	OnOwnerCharacterChanged(PreviousCharacter, OwnerCharacter);

	if (!OwnerCharacter || !MovementComponent)
	{
		AirState = EMMAirState::Grounded;
		return false;
	}

	return true;
}

void ULastFPSCharacterAnimInstance::CacheThreadSafeInputs()
{
	if (!OwnerCharacter || !MovementComponent)
	{
		bHasThreadSafeInputs = false;
		return;
	}

	CachedActorLocation = OwnerCharacter->GetActorLocation();
	CachedActorForwardVector = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	CachedActorRotation = OwnerCharacter->GetActorRotation();
	CachedDirectionBaseRotation = CachedActorRotation;
	CachedVelocity = MovementComponent->Velocity;
	CachedAcceleration = MovementComponent->GetCurrentAcceleration();
	bCachedHasMovementComponent = true;
	bCachedIsMovingOnGround = MovementComponent->IsMovingOnGround();
	bCachedOrientToMovement = MovementComponent->bOrientRotationToMovement;
	bCachedIsFalling = MovementComponent->IsFalling();
	bCachedIsCrouching = MovementComponent->IsCrouching();
	bCachedUseSeparateBrakingFriction = MovementComponent->bUseSeparateBrakingFriction;
	CachedGroundFriction = MovementComponent->GroundFriction;
	CachedBrakingFriction = MovementComponent->BrakingFriction;
	CachedBrakingFrictionFactor = MovementComponent->BrakingFrictionFactor;
	CachedMaxBrakingDeceleration = MovementComponent->GetMaxBrakingDeceleration();
	CachedBrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;
	CachedMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
	CachedGravityZ = MovementComponent->GetGravityZ();
	bCachedIsSprinting = false;
	bCachedWantsToSprint = false;
	bCachedWantsToWalk = false;
	bCachedHasMovementInput = false;
	CachedMoveInput = FVector2D::ZeroVector;
	CacheMovementModeFlags();
	if (bCachedHasMovementInput)
	{
		EMMCardinalDirection InputCardinalDirection = EMMCardinalDirection::Forward;
		if (TryGetMoveInputCardinalDirection(InputCardinalDirection))
		{
			StartCardinalDirection = InputCardinalDirection;
		}
	}
	bHasThreadSafeInputs = true;
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeDisplacement(float DeltaSeconds)
{
	DisplacementSinceLastUpdate = 0.f;
	DisplacementSpeed = 0.f;

	if (!bHasPreviousActorLocation || FMath::IsNearlyZero(DeltaSeconds))
	{
		PreviousActorLocation = CachedActorLocation;
		bHasPreviousActorLocation = true;
		return;
	}

	const FVector DisplacementDelta = CachedActorLocation - PreviousActorLocation;
	DisplacementSinceLastUpdate = DisplacementDelta.Size2D();
	DisplacementSpeed = DisplacementSinceLastUpdate / DeltaSeconds;
	PreviousActorLocation = CachedActorLocation;
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeLocomotionState(float DeltaSeconds)
{
	UpdateThreadSafeMovementValues();

	const bool bHasStartDirection = UpdateThreadSafeDirectionValues();
	const bool bHasStartIntent = bHasStartDirection || bCachedHasMovementInput;

	UpdateThreadSafeStartRequest(DeltaSeconds, bHasStartIntent);
	UpdateThreadSafeCardinalDirections();
	UpdateThreadSafeOrientationWarping();
	UpdateThreadSafeLocomotionMode();
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeMovementValues()
{
	Velocity = CachedVelocity;
	CharacterVelocity2D = FVector(Velocity.X, Velocity.Y, 0.f);
	LocalVelocity = CachedActorRotation.UnrotateVector(CharacterVelocity2D);
	Speed = Velocity.Size2D();
	LocomotionDirection = Speed > KINDA_SMALL_NUMBER
		                      ? Velocity.GetSafeNormal2D()
		                      : CachedActorForwardVector;
	bIsSprinting = bCachedIsSprinting;
	bWantsToSprint = bCachedWantsToSprint;
	bWantsToWalk = bCachedWantsToWalk;
	bHasMovementInput = bCachedHasMovementInput;
	bHasAcceleration = CachedAcceleration.SizeSquared2D() > KINDA_SMALL_NUMBER;
	bHasVelocity = !FMath::IsNearlyEqual(LocalVelocity.SizeSquared2D(), 0.f, VelocityNearlyZeroTolerance);
	LocomotionAngle = bHasVelocity ? GetVelocityDirectionAngle() : 0.f;
	bDirectionBaseAligned = FMath::Abs(FMath::FindDeltaAngleDegrees(
		                        CachedActorRotation.Yaw,
		                        CachedDirectionBaseRotation.Yaw)) <= DirectionBaseAlignmentTolerance;
}

bool ULastFPSCharacterAnimInstance::UpdateThreadSafeDirectionValues()
{
	Direction = LocomotionAngle;
	StartDirection = GetStartDirectionAngle();

	return bHasVelocity || bHasAcceleration;
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeCardinalDirections()
{
	if (!bCachedOrientToMovement)
	{
		// 무기 장착(strafe): 카메라 고정이라 입력 방향이 곧 start 방향 (A→Left, D→Right).
		EMMCardinalDirection InputCardinal = EMMCardinalDirection::Forward;
		if (bHasLatchedStartCardinalDirection)
		{
			StartCardinalDirection = LatchedStartCardinalDirection;
		}
		else if (TryGetMoveInputCardinalDirection(InputCardinal))
		{
			StartCardinalDirection = InputCardinal;
		}
		else
		{
			StartCardinalDirection = DirectionEMM(StartDirection);
		}
	}
	else
	{
		// 무기 미장착(orient to movement): 캐릭터가 이동방향으로 회전하므로 start는 항상 forward.
		StartCardinalDirection = EMMCardinalDirection::Forward;
	}

	StopCardinalDirection = DirectionEMM(LocomotionAngle);
	LocomotionCardinalDirection = Speed >= CardinalDirectionMinSpeed
		                              ? DirectionToStableCardinalDirection(
			                              LocomotionAngle,
			                              LocomotionCardinalDirection,
			                              CardinalDirectionHysteresis)
		                              : EMMCardinalDirection::Forward;
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeOrientationWarping()
{
	OrientationWarpingAngle = Direction;
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeStartRequest(float DeltaSeconds, bool bHasStartIntent)
{
	if (!bHasStartIntent || IsUsingSprintLocomotion() || !bCachedIsMovingOnGround)
	{
		bStartRequestConsumed = false;
		StartRequestRemainingTime = 0.f;
		bHasLatchedStartCardinalDirection = false;
	}

	const bool bHasStartRequest = bHasStartIntent
		&& !IsUsingSprintLocomotion()
		&& bCachedIsMovingOnGround
		&& Speed <= StartingSpeedThreshold;
	if (bHasStartRequest && !bStartRequestConsumed)
	{
		bStartRequestConsumed = true;
		StartRequestRemainingTime = FMath::Max(StartRequestHoldTime, DeltaSeconds);
		bHasLatchedStartCardinalDirection =
			TryGetMoveInputCardinalDirection(LatchedStartCardinalDirection);
		if (!bHasLatchedStartCardinalDirection)
		{
			LatchedStartCardinalDirection = DirectionEMM(StartDirection);
			bHasLatchedStartCardinalDirection = true;
		}
	}
	else if (StartRequestRemainingTime > 0.f)
	{
		StartRequestRemainingTime = FMath::Max(0.f, StartRequestRemainingTime - DeltaSeconds);
	}

	bIsStarting = StartRequestRemainingTime > 0.f;
	if (!bIsStarting)
	{
		bHasLatchedStartCardinalDirection = false;
	}
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeLocomotionMode()
{
	const bool bShouldIdle = !bHasVelocity && !bHasAcceleration && !bHasMovementInput;

	if (IsUsingSprintLocomotion() && (bHasAcceleration || bHasMovementInput))
	{
		LocomotionState = EMMLocomotionState::Sprint;
	}
	else if (bShouldIdle)
	{
		LocomotionState = EMMLocomotionState::Idle;
	}
	else if (bWantsToWalk)
	{
		LocomotionState = EMMLocomotionState::Walk;
	}
	else
	{
		LocomotionState = EMMLocomotionState::Jog;
	}
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeDistanceMatching(float DeltaSeconds)
{
	bShouldStop = false;

	if (!bCachedIsMovingOnGround)
	{
		bStopRequestConsumed = false;
		StopRequestRemainingTime = 0.f;
		DistanceToStop = 0.f;
		return;
	}

	if (bHasAcceleration || bHasMovementInput || bIsSprinting || bWantsToSprint)
	{
		bStopRequestConsumed = false;
		StopRequestRemainingTime = 0.f;
		DistanceToStop = 0.f;
		return;
	}

	const bool bCanPredictStop = Speed > 3.f;
	if (!bCanPredictStop)
	{
		if (StopRequestRemainingTime > 0.f)
		{
			StopRequestRemainingTime = FMath::Max(0.f, StopRequestRemainingTime - DeltaSeconds);
			bShouldStop = StopRequestRemainingTime > 0.f;
			return;
		}

		bStopRequestConsumed = false;
		DistanceToStop = 0.f;
		return;
	}

	const FVector Velocity2D(Velocity.X, Velocity.Y, 0.f);
	FVector VelocityDirection;
	float Speed2D = 0.f;
	Velocity2D.ToDirectionAndLength(VelocityDirection, Speed2D);

	if (Speed2D <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	float BrakingFriction = bCachedUseSeparateBrakingFriction
		                        ? CachedBrakingFriction
		                        : CachedGroundFriction;
	BrakingFriction = FMath::Max(0.f, BrakingFriction * CachedBrakingFrictionFactor);

	const float BrakingDeceleration = FMath::Max(0.f, CachedMaxBrakingDeceleration);
	const float Divisor = BrakingFriction * Speed2D + BrakingDeceleration;
	if (Divisor <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float TimeToStop = Speed2D / Divisor;
	const FVector PredictedStopOffset =
		Velocity2D * TimeToStop
		+ 0.5f * ((-BrakingFriction) * Velocity2D - BrakingDeceleration * VelocityDirection) * TimeToStop * TimeToStop;

	DistanceToStop = PredictedStopOffset.Size2D();
	const bool bHasStopRequest = Speed >= StopPredictionMinSpeed && DistanceToStop > KINDA_SMALL_NUMBER;

	if (bHasStopRequest && !bStopRequestConsumed)
	{
		bStopRequestConsumed = true;
		StopRequestRemainingTime = FMath::Max(StopRequestHoldTime, DeltaSeconds);
	}

	if (StopRequestRemainingTime > 0.f)
	{
		bShouldStop = true;
		StopRequestRemainingTime = FMath::Max(0.f, StopRequestRemainingTime - DeltaSeconds);
	}
}

void ULastFPSCharacterAnimInstance::UpdateGroundDistance()
{
	bHasGroundDistance = false;
	GroundDistance = 0.f;

	if (!OwnerCharacter || !MovementComponent)
	{
		return;
	}

	if (!MovementComponent->IsFalling())
	{
		bHasGroundDistance = true;
		return;
	}

	const UCapsuleComponent* CapsuleComponent = OwnerCharacter->GetCapsuleComponent();
	if (!CapsuleComponent || GroundDistanceTraceLength <= 0.f)
	{
		return;
	}

	const FVector ActorLocation = OwnerCharacter->GetActorLocation();
	const float CapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const FVector TraceStart = ActorLocation;
	const FVector TraceEnd = ActorLocation - FVector::UpVector * (CapsuleHalfHeight + GroundDistanceTraceLength);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LastFPSGroundDistanceTrace), false, OwnerCharacter);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	FHitResult HitResult;
	UWorld* World = OwnerCharacter->GetWorld();
	if (!World || !World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GroundDistanceTraceChannel, QueryParams))
	{
		GroundDistance = GroundDistanceTraceLength;
		return;
	}

	const float CapsuleBottomZ = ActorLocation.Z - CapsuleHalfHeight;
	GroundDistance = FMath::Max(0.f, CapsuleBottomZ - HitResult.ImpactPoint.Z);
	bHasGroundDistance = true;
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeYaw(float DeltaSeconds)
{
	const float CurrentActorYaw = CachedActorRotation.Yaw;

	if (!bHasPreviousActorYaw || FMath::IsNearlyZero(DeltaSeconds))
	{
		PreviousActorYaw = CurrentActorYaw;
		bHasPreviousActorYaw = true;
		Yaw = 0.f;
		return;
	}

	const float DeltaYaw = FMath::FindDeltaAngleDegrees(PreviousActorYaw, CurrentActorYaw);
	float TargetYaw = -DeltaYaw / DeltaSeconds;
	if (FMath::Abs(TargetYaw) <= YawDeadZone)
	{
		TargetYaw = 0.f;
	}

	Yaw = FMath::FInterpTo(Yaw, TargetYaw, DeltaSeconds, YawInterpSpeed);
	PreviousActorYaw = CurrentActorYaw;
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeAirState()
{
	if (!bCachedIsFalling)
	{
		AirState = EMMAirState::Grounded;
	}
	else
	{
		AirState = CachedVelocity.Z > 0.f ? EMMAirState::Jumping : EMMAirState::Falling;
	}
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafeStance()
{
	Stance = bCachedIsCrouching
		         ? EMMStance::Crouching
		         : EMMStance::Standing;
}

void ULastFPSCharacterAnimInstance::UpdateCombatState()
{
	const ALastFPSCharacterBase* Base = Cast<ALastFPSCharacterBase>(OwnerCharacter);
	if (!Base)
	{
		bIsDead = false;
		bIsInCombat = false;
		AimMode = EMMAimMode::Hip;
		return;
	}

	bIsDead = !Base->IsAlive();
	bIsInCombat = Base->IsInCombat();
	AimMode = Base->GetIsADS() ? EMMAimMode::ADS : EMMAimMode::Hip;

	if (AController* Controller = OwnerCharacter->GetController())
	{
		const FRotator ControlRot = Controller->GetControlRotation();
		const FRotator ActorRot = OwnerCharacter->GetActorRotation();
		AimPitch = FMath::FindDeltaAngleDegrees(ActorRot.Pitch, ControlRot.Pitch);
		AimYaw = FMath::FindDeltaAngleDegrees(ActorRot.Yaw, ControlRot.Yaw);
	}
}

void ULastFPSCharacterAnimInstance::UpdateThreadSafePivot()
{
	if (Velocity.SizeSquared2D() < PivotMinSpeed * PivotMinSpeed
		|| CachedAcceleration.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		bIsPivoting = false;
		return;
	}

	const FVector VelDir = Velocity.GetSafeNormal2D();
	const FVector AccDir = CachedAcceleration.GetSafeNormal2D();
	bIsPivoting = FVector::DotProduct(VelDir, AccDir) < PivotDotThreshold;
}

bool ULastFPSCharacterAnimInstance::TryGetMoveInputCardinalDirection(EMMCardinalDirection& OutDirection) const
{
	if (CachedMoveInput.SizeSquared() <= MovementInputDeadZone * MovementInputDeadZone)
	{
		return false;
	}

	if (FMath::Abs(CachedMoveInput.X) > FMath::Abs(CachedMoveInput.Y))
	{
		OutDirection = CachedMoveInput.X < 0.f
			               ? EMMCardinalDirection::Left
			               : EMMCardinalDirection::Right;
		return true;
	}

	OutDirection = CachedMoveInput.Y < 0.f
		               ? EMMCardinalDirection::Back
		               : EMMCardinalDirection::Forward;
	return true;
}

float ULastFPSCharacterAnimInstance::GetDirectionAngleFromVector(const FVector& DirectionVector) const
{
	if (DirectionVector.SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const FRotator VelocityRotation = DirectionVector.ToOrientationRotator();
	const float DirectionFromCurrentActor = FMath::FindDeltaAngleDegrees(
		CachedActorRotation.Yaw,
		VelocityRotation.Yaw);
	const float RemainingBaseYaw = FMath::FindDeltaAngleDegrees(
		CachedActorRotation.Yaw,
		CachedDirectionBaseRotation.Yaw);

	return FRotator::NormalizeAxis(DirectionFromCurrentActor - RemainingBaseYaw);
}

float ULastFPSCharacterAnimInstance::GetVelocityDirectionAngle() const
{
	return GetDirectionAngleFromVector(Velocity);
}

float ULastFPSCharacterAnimInstance::GetStartDirectionAngle() const
{
	if (bHasAcceleration)
	{
		return GetDirectionAngleFromVector(CachedAcceleration);
	}

	if (bHasVelocity)
	{
		return GetDirectionAngleFromVector(Velocity);
	}

	return 0.f;
}

bool ULastFPSCharacterAnimInstance::IsUsingSprintLocomotion() const
{
	return bIsSprinting || bWantsToSprint;
}
