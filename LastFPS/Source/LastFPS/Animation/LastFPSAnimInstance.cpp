#include "Animation/LastFPSAnimInstance.h"

#include "Character/LastFPSHero.h"
#include "Character/Components/LastFPSGrapplingAnimationComponent.h"
#include "Character/Components/WeaponComponent.h"
#include "Character/Interfaces/LastFPSWeaponUser.h"

namespace
{
	UWeaponComponent* ResolveWeaponComponent(const ACharacter* Character)
	{
		const ILastFPSWeaponUser* WeaponUser = Cast<ILastFPSWeaponUser>(Character);
		return WeaponUser ? WeaponUser->GetWeaponComponent() : nullptr;
	}
}

void ULastFPSAnimInstance::ResetAnimState()
{
	Super::ResetAnimState();

	bJumpStartRequested = false;
	bJumpApexRequested = false;
	JumpStartSequence = 0;
	ObservedJumpStartSequence = 0;
	CachedJumpStartSequence = 0;
	bHasObservedJumpStartSequence = false;
	JumpStartRequestRemainingTime = 0.f;
	WeaponType = EMMWeaponType::Unarmed;
	CombatState = EMMCombatState::Idle;
	bIsFiring = false;
	bIsCasting = false;
	LeftHandIKAlpha = 0.f;
	LeftHandIKTransform = FTransform::Identity;
	LeftHandIKJointTargetLocation = FVector::ZeroVector;
	GrapplingIKEffectorLocation = FVector::ZeroVector;
	GrapplingIKJointTargetLocation = FVector::ZeroVector;
	GrapplingIKHandRotation = FRotator::ZeroRotator;
	GrapplingIKAlpha = 0.f;
	GrapplingBodyPitch = 0.f;
	GrapplingAnimationPhase = ELastFPSGrapplingAnimationPhase::None;
	GrapplingHookFlightDuration = 0.f;
}

void ULastFPSAnimInstance::OnOwnerCharacterChanged(ACharacter* PreviousCharacter, ACharacter* NewCharacter)
{
	if (UWeaponComponent* PreviousWeapon = ResolveWeaponComponent(PreviousCharacter))
	{
		PreviousWeapon->OnWeaponEquippedChanged.RemoveDynamic(this, &ULastFPSAnimInstance::OnWeaponEquipped);
	}

	WeaponType = EMMWeaponType::Unarmed;

	if (UWeaponComponent* Weapon = ResolveWeaponComponent(NewCharacter))
	{
		WeaponType = Weapon->GetWeaponType();
		Weapon->OnWeaponEquippedChanged.AddUniqueDynamic(this, &ULastFPSAnimInstance::OnWeaponEquipped);
	}
}

void ULastFPSAnimInstance::CacheMovementModeFlags()
{
	if (const ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
	{
		CachedMoveInput = Hero->GetCachedMoveInput();
		bCachedIsSprinting = Hero->GetIsSprinting();
		bCachedWantsToSprint = Hero->GetWantsToSprint();
		bCachedWantsToWalk = Hero->GetWantsToWalk();
		bCachedHasMovementInput = CachedMoveInput.SizeSquared() > MovementInputDeadZone * MovementInputDeadZone;
		SetCachedDirectionBaseRotation(Hero->GetLocomotionDirectionBaseRotation());
	}
}

void ULastFPSAnimInstance::UpdateGameThreadCharacterState(float)
{
	if (const ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
	{
		CachedJumpStartSequence = Hero->GetJumpStartSequence();
	}
	else
	{
		CachedJumpStartSequence = 0;
	}

	UpdateHandIK();
	UpdateGrapplingIK();
}

void ULastFPSAnimInstance::UpdateThreadSafeCharacterState(float DeltaSeconds)
{
	UpdateJumpStartRequest(DeltaSeconds);
}

void ULastFPSAnimInstance::UpdateCombatState()
{
	Super::UpdateCombatState();

	if (const ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
	{
		CombatState = Hero->GetCombatState();
		bIsFiring = CombatState == EMMCombatState::Attacking;
		bIsCasting = CombatState == EMMCombatState::Casting;
		return;
	}

	CombatState = EMMCombatState::Idle;
	bIsFiring = false;
	bIsCasting = false;
}

void ULastFPSAnimInstance::UpdateJumpStartRequest(float DeltaSeconds)
{
	bJumpStartRequested = false;
	bJumpApexRequested = false;

	JumpStartSequence = CachedJumpStartSequence;
	if (JumpStartSequence <= 0 && !bHasObservedJumpStartSequence)
	{
		ObservedJumpStartSequence = 0;
		bHasObservedJumpStartSequence = false;
		JumpStartRequestRemainingTime = 0.f;
		return;
	}

	if (!bHasObservedJumpStartSequence)
	{
		ObservedJumpStartSequence = JumpStartSequence;
		bHasObservedJumpStartSequence = true;
		return;
	}

	if (ObservedJumpStartSequence != JumpStartSequence)
	{
		ObservedJumpStartSequence = JumpStartSequence;
		JumpStartRequestRemainingTime = FMath::Max(JumpStartRequestHoldTime, DeltaSeconds);
	}
	else if (JumpStartRequestRemainingTime > 0.f)
	{
		JumpStartRequestRemainingTime = FMath::Max(0.f, JumpStartRequestRemainingTime - DeltaSeconds);
	}

	const bool bHasActiveJumpRequest = JumpStartRequestRemainingTime > 0.f;
	bJumpStartRequested = bHasActiveJumpRequest;
	bJumpApexRequested = bHasActiveJumpRequest;
}

void ULastFPSAnimInstance::UpdateHandIK()
{
	LeftHandIKAlpha = 0.f;
	LeftHandIKTransform = FTransform::Identity;
	LeftHandIKJointTargetLocation = WeaponLeftHandIKJointTargetLocation;

	if (CombatState == EMMCombatState::Casting)
	{
		return;
	}
	if (!OwnerCharacter || !OwnerCharacter->GetMesh())
	{
		return;
	}

	UWeaponComponent* Weapon = ResolveWeaponComponent(OwnerCharacter);
	if (CombatState == EMMCombatState::Reloading && bUseReloadLeftHandIKTarget)
	{
		if (!Weapon || !Weapon->HasWeapon() || Weapon->ReloadLeftHandIKTargetName.IsNone())
		{
			return;
		}

		FTransform ReloadIKTransform;
		const bool bHasReloadIKTransform = Weapon->GetLeftHandIKTransformForTarget(
			Weapon->ReloadLeftHandIKTargetName,
			OwnerCharacter->GetMesh(),
			RightHandBoneName,
			ReloadIKTransform);

		if (bHasReloadIKTransform)
		{
			LeftHandIKTransform = ReloadIKTransform;
			LeftHandIKAlpha = 1.f;
		}
		return;
	}

	const bool bUseActiveCombatGrip = bIsInCombat || AimMode == EMMAimMode::ADS;
	if (!bUseActiveCombatGrip && Weapon && Weapon->HasWeapon()
		&& !Weapon->ReadyLeftHandIKSocketName.IsNone())
	{
		FTransform ReadyIKTransform;
		const bool bHasReadyIKTransform = Weapon->GetLeftHandIKTransformForTarget(
			Weapon->ReadyLeftHandIKSocketName,
			OwnerCharacter->GetMesh(),
			RightHandBoneName,
			ReadyIKTransform);

		if (bHasReadyIKTransform)
		{
			LeftHandIKTransform = ReadyIKTransform;
			LeftHandIKAlpha = 1.f;
			return;
		}
	}

	// ADS 또는 전투 중에는 기존 기본 소켓을 사용한다.
	LeftHandIKTransform = WeaponLeftHandIKTransform;
	LeftHandIKAlpha = WeaponLeftHandIKAlpha;
}

void ULastFPSAnimInstance::UpdateGrapplingIK()
{
	GrapplingIKEffectorLocation = FVector::ZeroVector;
	GrapplingIKJointTargetLocation = FVector::ZeroVector;
	GrapplingIKHandRotation = FRotator::ZeroRotator;
	GrapplingIKAlpha = 0.f;
	GrapplingBodyPitch = 0.f;
	GrapplingAnimationPhase = ELastFPSGrapplingAnimationPhase::None;
	GrapplingHookFlightDuration = 0.f;

	const ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter);
	const ULastFPSGrapplingAnimationComponent* AnimationComponent = Hero
		? Hero->GetGrapplingAnimationComponent()
		: nullptr;
	if (!AnimationComponent)
	{
		return;
	}

	GrapplingIKEffectorLocation = AnimationComponent->GetGrapplingIKEffectorLocation();
	GrapplingIKJointTargetLocation = AnimationComponent->GetGrapplingIKJointTargetLocation();
	GrapplingIKHandRotation = AnimationComponent->GetGrapplingIKHandRotation();
	GrapplingIKAlpha = AnimationComponent->GetGrapplingIKAlpha();
	GrapplingBodyPitch = AnimationComponent->GetGrapplingBodyPitch();
	GrapplingAnimationPhase = AnimationComponent->GetGrapplingAnimationPhase();
	GrapplingHookFlightDuration = AnimationComponent->GetHookFlightDuration();
}

void ULastFPSAnimInstance::OnWeaponEquipped(bool)
{
	if (const UWeaponComponent* Weapon = ResolveWeaponComponent(OwnerCharacter))
	{
		WeaponType = Weapon->GetWeaponType();
		return;
	}

	WeaponType = EMMWeaponType::Unarmed;
}
