#include "Animation/LastFPSAnimInstance.h"

#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"

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
}

void ULastFPSAnimInstance::OnOwnerCharacterChanged(ACharacter* PreviousCharacter, ACharacter* NewCharacter)
{
	if (ALastFPSHero* PreviousHero = Cast<ALastFPSHero>(PreviousCharacter))
	{
		if (UWeaponComponent* PreviousWeapon = PreviousHero->GetWeaponComponent())
		{
			PreviousWeapon->OnWeaponEquippedChanged.RemoveDynamic(this, &ULastFPSAnimInstance::OnWeaponEquipped);
		}
	}

	WeaponType = EMMWeaponType::Unarmed;

	if (ALastFPSHero* NewHero = Cast<ALastFPSHero>(NewCharacter))
	{
		if (UWeaponComponent* Weapon = NewHero->GetWeaponComponent())
		{
			WeaponType = Weapon->GetWeaponType();
			Weapon->OnWeaponEquippedChanged.AddUniqueDynamic(this, &ULastFPSAnimInstance::OnWeaponEquipped);
		}
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
			                             Weapon->ReloadLeftHandIKTargetName,
			                             Hero->GetMesh(),
			                             RightHandBoneName,
			                             IKTransform)
		                             : Weapon->GetLeftHandIKTransform(
			                             Hero->GetMesh(),
			                             RightHandBoneName,
			                             IKTransform);

	if (bHasIKTransform)
	{
		LeftHandIKTransform = IKTransform;
		LeftHandIKAlpha = 1.f;
	}
}

void ULastFPSAnimInstance::OnWeaponEquipped(bool)
{
	if (const ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
	{
		if (const UWeaponComponent* Weapon = Hero->GetWeaponComponent())
		{
			WeaponType = Weapon->GetWeaponType();
			return;
		}
	}

	WeaponType = EMMWeaponType::Unarmed;
}
