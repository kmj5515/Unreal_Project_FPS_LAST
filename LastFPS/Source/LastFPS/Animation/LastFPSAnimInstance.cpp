#include "Animation/LastFPSAnimInstance.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void ULastFPSAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    OwnerCharacter = Cast<ACharacter>(GetOwningActor());
    if (OwnerCharacter)
    {
        MovementComponent = OwnerCharacter->GetCharacterMovement();

        if (ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
        {
            if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
            {
                WeaponType = Weapon->GetWeaponType();
                Weapon->OnWeaponEquippedChanged.AddUniqueDynamic(this, &ULastFPSAnimInstance::OnWeaponEquipped);
            }
        }
    }
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

    Super::NativeUninitializeAnimation();
}

void ULastFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!OwnerCharacter || !MovementComponent)
    {
        return;
    }

    UpdateYaw(DeltaSeconds);
    UpdateLocomotionState();
    UpdateDistanceMatching();
    UpdateAirState();
    UpdateStance();
    UpdateCombatState();
    UpdatePivot();
    UpdateHandIK();
}

void ULastFPSAnimInstance::UpdateLocomotionState()
{
    Velocity = MovementComponent->Velocity;
    Speed = Velocity.Size2D();
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

EMMCardinalDirection ULastFPSAnimInstance::DirectionToCardinalDirection(float InDirection)
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
    if (MovementComponent->IsFalling())
    {
        AirState = (MovementComponent->Velocity.Z > 0.f)
            ? EMMAirState::Jumping
            : EMMAirState::Falling;
    }
    else
    {
        AirState = EMMAirState::Grounded;
    }
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

    // Aim Pitch / Yaw: 컨트롤러 회전과 캐릭터 회전의 차이를 -180~180으로 정규화
    if (AController* Controller = OwnerCharacter->GetController())
    {
        const FRotator ControlRot = Controller->GetControlRotation();
        const FRotator ActorRot   = OwnerCharacter->GetActorRotation();
        const FRotator Delta      = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, ActorRot);
        AimPitch = Delta.Pitch;
        AimYaw   = Delta.Yaw;
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
        ? Weapon->GetLeftHandIKTransformForTarget(Weapon->ReloadLeftHandIKTargetName, Hero->GetMesh(), RightHandBoneName, IKTransform)
        : Weapon->GetLeftHandIKTransform(Hero->GetMesh(), RightHandBoneName, IKTransform);

    if (bHasIKTransform)
    {
        LeftHandIKTransform = IKTransform;
        LeftHandIKAlpha = 1.f;
    }
}
