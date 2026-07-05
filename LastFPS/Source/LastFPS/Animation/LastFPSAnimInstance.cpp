#include "Animation/LastFPSAnimInstance.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void ULastFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	RefreshOwnerReferences();
}

void ULastFPSAnimInstance::NativeUninitializeAnimation()
{
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
	{
		if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
		{
			Weapon->OnWeaponEquippedChanged.RemoveDynamic(this, &ULastFPSAnimInstance::OnWeaponEquipped);
		}
	}

	OwnerCharacter = nullptr;
	MovementComponent = nullptr;
	DisplacementSinceLastUpdate = FVector::ZeroVector;
	DisplacementSpeed = 0.f;
	bHasPreviousActorLocation = false;

	Super::NativeUninitializeAnimation();
}

void ULastFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!RefreshOwnerReferences())
	{
		return;
	}

	UpdateDisplacement(DeltaSeconds);
	UpdateYaw(DeltaSeconds);
	UpdateLocomotionState();
	UpdateDistanceMatching();
	UpdateAirState();
	UpdateGroundDistance();
	UpdateStance();
	UpdateCombatState();
	UpdatePivot();
	UpdateHandIK();
}

bool ULastFPSAnimInstance::RefreshOwnerReferences()
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

	if (ALastFPSHero* PreviousHero = Cast<ALastFPSHero>(OwnerCharacter))
	{
		if (UWeaponComponent* PreviousWeapon = PreviousHero->GetWeaponComponent())
		{
			PreviousWeapon->OnWeaponEquippedChanged.RemoveDynamic(this, &ULastFPSAnimInstance::OnWeaponEquipped);
		}
	}

	OwnerCharacter = CurrentCharacter;
	MovementComponent = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
	PreviousActorLocation = OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
	bHasPreviousActorLocation = false;

	if (!OwnerCharacter || !MovementComponent)
	{
		AirState = EMMAirState::Grounded;
		DisplacementSinceLastUpdate = FVector::ZeroVector;
		DisplacementSpeed = 0.f;
		return false;
	}

	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
	{
		if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
		{
			WeaponType = Weapon->GetWeaponType();
			Weapon->OnWeaponEquippedChanged.AddUniqueDynamic(this, &ULastFPSAnimInstance::OnWeaponEquipped);
		}
	}
	else
	{
		WeaponType = EMMWeaponType::Unarmed;
	}

	return true;
}

void ULastFPSAnimInstance::UpdateDisplacement(float DeltaSeconds)
{
	DisplacementSinceLastUpdate = FVector::ZeroVector;
	DisplacementSpeed = 0.f;

	if (!OwnerCharacter)
	{
		bHasPreviousActorLocation = false;
		return;
	}

	const FVector CurrentActorLocation = OwnerCharacter->GetActorLocation();
	if (!bHasPreviousActorLocation || FMath::IsNearlyZero(DeltaSeconds))
	{
		PreviousActorLocation = CurrentActorLocation;
		bHasPreviousActorLocation = true;
		return;
	}

	DisplacementSinceLastUpdate = CurrentActorLocation - PreviousActorLocation;
	DisplacementSpeed = DisplacementSinceLastUpdate.Size2D() / DeltaSeconds;
	PreviousActorLocation = CurrentActorLocation;
}

void ULastFPSAnimInstance::UpdateLocomotionState()
{
	Velocity = MovementComponent->Velocity;
	Speed = Velocity.Size2D();
	LocomotionDirection = Speed > KINDA_SMALL_NUMBER
		                      ? Velocity.GetSafeNormal2D()
		                      : OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	bIsSprinting = false;
	bWantsToSprint = false;
	const FVector Acceleration = MovementComponent->GetCurrentAcceleration();
	bHasAcceleration = Acceleration.SizeSquared2D() > KINDA_SMALL_NUMBER;

	if (Speed > 1.f)
	{
		FRotator ActorRotation = OwnerCharacter->GetActorRotation();
		FRotator VelocityRotation = UKismetMathLibrary::MakeRotFromX(Velocity);
		Direction = UKismetMathLibrary::NormalizedDeltaRotator(VelocityRotation, ActorRotation).Yaw;
	}
	else
	{
		Direction = 0.f;
	}

	if (bHasAcceleration)
	{
		const FRotator ActorRotation = OwnerCharacter->GetActorRotation();
		const FRotator AccelerationRotation = UKismetMathLibrary::MakeRotFromX(Acceleration);
		StartDirection = UKismetMathLibrary::NormalizedDeltaRotator(AccelerationRotation, ActorRotation).Yaw;
	}
	else
	{
		StartDirection = Direction;
	}

	StartCardinalDirection = DirectionToCardinalDirection(StartDirection);
	StopCardinalDirection = DirectionToCardinalDirection(Direction);
	LocomotionCardinalDirection = Speed >= CardinalDirectionMinSpeed
		                              ? DirectionToStableCardinalDirection(Direction, LocomotionCardinalDirection)
		                              : EMMCardinalDirection::Forward;

	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
	{
		bIsSprinting = Hero->GetIsSprinting();
		bWantsToSprint = Hero->GetWantsToSprint();
	}

	if (bHasAcceleration || bIsSprinting || bWantsToSprint)
	{
		OrientationWarpingAngle = Direction;
	}
	else if (Speed <= 1.f)
	{
		OrientationWarpingAngle = 0.f;
	}

	const bool bUseSprintLocomotion = bIsSprinting || bWantsToSprint;
	bIsStarting = bHasAcceleration && !bUseSprintLocomotion && Speed <= StartingSpeedThreshold;

	if (bUseSprintLocomotion && bHasAcceleration)
	{
		LocomotionState = EMMLocomotionState::Sprint;
	}
	else if (Speed <= 3.f)
	{
		LocomotionState = EMMLocomotionState::Idle;
	}
	else
	{
		LocomotionState = EMMLocomotionState::Jog;
	}
}

void ULastFPSAnimInstance::UpdateDistanceMatching()
{
	bShouldStop = false;
	DistanceToStop = 0.f;

	if (!MovementComponent->IsMovingOnGround())
	{
		return;
	}

	if (!bHasAcceleration && !bIsSprinting && !bWantsToSprint && Speed > 1.f)
	{
		StopOrientationWarpingAngle = Direction;
		OrientationWarpingAngle = StopOrientationWarpingAngle;
	}

	const bool bCanPredictStop = !bHasAcceleration
		&& !bIsSprinting
		&& !bWantsToSprint
		&& Speed > 3.f;

	if (!bCanPredictStop)
	{
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

	float BrakingFriction = MovementComponent->bUseSeparateBrakingFriction
		                        ? MovementComponent->BrakingFriction
		                        : MovementComponent->GroundFriction;
	BrakingFriction = FMath::Max(0.f, BrakingFriction * MovementComponent->BrakingFrictionFactor);

	const float BrakingDeceleration = FMath::Max(0.f, MovementComponent->GetMaxBrakingDeceleration());
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
	bShouldStop = Speed >= StopPredictionMinSpeed && DistanceToStop > KINDA_SMALL_NUMBER;
}

void ULastFPSAnimInstance::UpdateGroundDistance()
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

EMMCardinalDirection ULastFPSAnimInstance::DirectionToCardinalDirection(float InDirection) const
{
	const float NormalizedDirection = FMath::Fmod(InDirection + 360.f, 360.f);
	const int32 DirectionIndex = FMath::FloorToInt((NormalizedDirection + 45.f) / 90.f) % 4;

	switch (DirectionIndex)
	{
	case 1:
		return EMMCardinalDirection::Right;
	case 2:
		return EMMCardinalDirection::Back;
	case 3:
		return EMMCardinalDirection::Left;
	case 0:
	default:
		return EMMCardinalDirection::Forward;
	}
}

EMMCardinalDirection ULastFPSAnimInstance::DirectionToStableCardinalDirection(
	float InDirection,
	EMMCardinalDirection CurrentDirection) const
{
	const float CurrentDirectionAngle = CardinalDirectionToAngle(CurrentDirection);
	const float DeltaFromCurrent = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentDirectionAngle, InDirection));
	const float KeepCurrentThreshold = 45.f + CardinalDirectionHysteresis;

	if (DeltaFromCurrent <= KeepCurrentThreshold)
	{
		return CurrentDirection;
	}

	return DirectionToCardinalDirection(InDirection);
}

float ULastFPSAnimInstance::CardinalDirectionToAngle(EMMCardinalDirection InDirection) const
{
	switch (InDirection)
	{
	case EMMCardinalDirection::Right:
		return 90.f;
	case EMMCardinalDirection::Back:
		return 180.f;
	case EMMCardinalDirection::Left:
		return -90.f;
	case EMMCardinalDirection::Forward:
	default:
		return 0.f;
	}
}

void ULastFPSAnimInstance::UpdateYaw(float DeltaSeconds)
{
	const float CurrentActorYaw = OwnerCharacter->GetActorRotation().Yaw;

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

void ULastFPSAnimInstance::UpdateAirState()
{
	const EMMAirState PreviousAirState = AirState;
	AirState = ResolveAirState(*MovementComponent);

	if (bDebugAirState && (!bHasLoggedAirState || PreviousAirState != AirState))
	{
		UE_LOG(LogTemp, Warning, TEXT("Air State Update : Air State : %s"), *GetAirStateName(AirState));
		bHasLoggedAirState = true;
	}
}

EMMAirState ULastFPSAnimInstance::ResolveAirState(const UCharacterMovementComponent& InMovementComponent)
{
	if (!InMovementComponent.IsFalling())
	{
		return EMMAirState::Grounded;
	}

	return InMovementComponent.Velocity.Z > 0.f
		       ? EMMAirState::Jumping
		       : EMMAirState::Falling;
}

FString ULastFPSAnimInstance::GetAirStateName(EMMAirState InAirState) const
{
	const UEnum* AirStateEnum = StaticEnum<EMMAirState>();
	return AirStateEnum
		       ? AirStateEnum->GetNameStringByValue(static_cast<int64>(InAirState))
		       : TEXT("Unknown");
}

void ULastFPSAnimInstance::UpdateStance()
{
	Stance = MovementComponent->IsCrouching()
		         ? EMMStance::Crouching
		         : EMMStance::Standing;
}

void ULastFPSAnimInstance::UpdateCombatState()
{
	ALastFPSCharacterBase* Base = Cast<ALastFPSCharacterBase>(OwnerCharacter);
	if (!Base)
	{
		return;
	}

	bIsDead = !Base->IsAlive();
	AimMode = Base->GetIsADS() ? EMMAimMode::ADS : EMMAimMode::Hip;

	// 컨트롤러 회전과 캐릭터 회전 차이를 -180~180 범위로 정규화한다.
	if (AController* Controller = OwnerCharacter->GetController())
	{
		const FRotator ControlRot = Controller->GetControlRotation();
		const FRotator ActorRot = OwnerCharacter->GetActorRotation();
		const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, ActorRot);
		AimPitch = Delta.Pitch;
		AimYaw = Delta.Yaw;
	}

	//UE_LOG(LogTemp, Log, TEXT("AimPitch: %f"), AimPitch);
	//UE_LOG(LogTemp, Log, TEXT("AimYaw: %f"), AimYaw);

	// 발사 중 여부
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(Base))
	{
		CombatState = Hero->GetCombatState();
		bIsFiring = CombatState == EMMCombatState::Attacking;
		bIsCasting = CombatState == EMMCombatState::Casting;
	}
	else
	{
		CombatState = EMMCombatState::Idle;
		bIsFiring = false;
		bIsCasting = false;
	}
}

void ULastFPSAnimInstance::OnWeaponEquipped(bool bEquipped)
{
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
	{
		if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
		{
			WeaponType = Weapon->GetWeaponType();
			return;
		}
	}
	WeaponType = EMMWeaponType::Unarmed;
}

void ULastFPSAnimInstance::UpdatePivot()
{
	const FVector Acc = MovementComponent->GetCurrentAcceleration();
	if (Velocity.SizeSquared2D() < PivotMinSpeed * PivotMinSpeed
		|| Acc.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		bIsPivoting = false;
		return;
	}

	const FVector VelDir = Velocity.GetSafeNormal2D();
	const FVector AccDir = Acc.GetSafeNormal2D();
	bIsPivoting = FVector::DotProduct(VelDir, AccDir) < PivotDotThreshold;
}

void ULastFPSAnimInstance::UpdateHandIK()
{
	LeftHandIKAlpha = 0.f;
	LeftHandIKTransform = FTransform::Identity;

	ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter);
	if (!Hero || !Hero->GetMesh())
	{
		return;
	}

	UWeaponComponent* Weapon = Hero->GetWeaponComponent();
	if (!Weapon || !Weapon->HasWeapon())
	{
		return;
	}

	if (CombatState == EMMCombatState::Casting)
	{
		return;
	}

	FTransform IKTransform;
	const bool bUseReloadTarget = bUseReloadLeftHandIKTarget
		&& CombatState == EMMCombatState::Reloading
		&& !Weapon->ReloadLeftHandIKTargetName.IsNone();

	const bool bHasIKTransform = bUseReloadTarget
		                             ? Weapon->GetLeftHandIKTransformForTarget(
			                             Weapon->ReloadLeftHandIKTargetName, Hero->GetMesh(), RightHandBoneName,
			                             IKTransform)
		                             : Weapon->GetLeftHandIKTransform(Hero->GetMesh(), RightHandBoneName, IKTransform);

	if (bHasIKTransform)
	{
		LeftHandIKTransform = IKTransform;
		LeftHandIKAlpha = 1.f;
	}
}
