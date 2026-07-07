#include "Animation/LastFPSHeroLinkedAnimLayer.h"
#include "Animation/LastFPSAnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"

namespace 
{
	FString GetLinkedLayerCardinalDirectionName(EMMCardinalDirection Direction)
	{
		const UEnum* DirectionEnum = StaticEnum<EMMCardinalDirection>();
		return DirectionEnum
				   ? DirectionEnum->GetNameStringByValue(static_cast<int64>(Direction))
				   : TEXT("Unknown");
	}
}

void ULastFPSHeroLinkedAnimLayer::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ULastFPSAnimInstance* HeroAnimInstance = GetHeroAnimInstance();
	if (!HeroAnimInstance)
	{
		CachedHeroLocomotionState = EMMLocomotionState::Idle;
		CachedHeroStartCardinalDirection = EMMCardinalDirection::Forward;
		CachedHeroStopCardinalDirection = EMMCardinalDirection::Forward;
		CachedHeroLocomotionCardinalDirection = EMMCardinalDirection::Forward;
		CachedHeroVelocity2D = FVector::ZeroVector;
		CachedHeroLocomotionAngle = 0.f;
		CachedHeroDisplacementSinceLastUpdate = 0.f;
		CachedHeroDisplacementSpeed = 0.f;
		bCachedHeroHasAcceleration = false;
		bCachedHeroHasVelocity = false;
		bCachedHeroShouldStop = false;
		bCachedHeroHasGroundDistance = false;
		CachedHeroDistanceToStop = 0.f;
		CachedHeroGroundDistance = 0.f;
		CachedHeroGroundFriction = 0.f;
		return;
	}

	CachedHeroLocomotionState = HeroAnimInstance->GetLocomotionState();
	CachedHeroStartCardinalDirection = HeroAnimInstance->GetStartCardinalDirection();
	CachedHeroStopCardinalDirection = HeroAnimInstance->GetStopCardinalDirection();
	CachedHeroLocomotionCardinalDirection = HeroAnimInstance->GetLocomotionCardinalDirection();
	CachedHeroVelocity2D = HeroAnimInstance->GetVelocity2D();
	CachedHeroLocomotionAngle = HeroAnimInstance->GetLocomotionAngle();
	CachedHeroDisplacementSinceLastUpdate = HeroAnimInstance->GetDisplacementSinceLastUpdate();
	CachedHeroDisplacementSpeed = HeroAnimInstance->GetDisplacementSpeed();
	bCachedHeroHasAcceleration = HeroAnimInstance->HasAcceleration();
	bCachedHeroHasVelocity = HeroAnimInstance->HasVelocity();
	bCachedHeroShouldStop = HeroAnimInstance->ShouldStop();
	bCachedHeroHasGroundDistance = HeroAnimInstance->HasGroundDistance();
	CachedHeroDistanceToStop = HeroAnimInstance->GetDistanceToStop();
	CachedHeroGroundDistance = HeroAnimInstance->GetGroundDistance();
	CachedHeroGroundFriction = HeroAnimInstance->GetGroundFriction();
}

ULastFPSAnimInstance* ULastFPSHeroLinkedAnimLayer::GetHeroAnimInstance() const
{
	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	return MeshComponent ? Cast<ULastFPSAnimInstance>(MeshComponent->GetAnimInstance()) : nullptr;
}

EMMLocomotionState ULastFPSHeroLinkedAnimLayer::GetHeroLocomotionState() const
{
	return CachedHeroLocomotionState;
}

EMMCardinalDirection ULastFPSHeroLinkedAnimLayer::GetHeroStartCardinalDirection() const
{
	return CachedHeroStartCardinalDirection;
}

EMMCardinalDirection ULastFPSHeroLinkedAnimLayer::GetHeroStopCardinalDirection() const
{
	return CachedHeroStopCardinalDirection;
}

EMMCardinalDirection ULastFPSHeroLinkedAnimLayer::GetHeroLocomotionCardinalDirection() const
{
	return CachedHeroLocomotionCardinalDirection;
}

FVector ULastFPSHeroLinkedAnimLayer::GetHeroVelocity2D() const
{
	return CachedHeroVelocity2D;
}

float ULastFPSHeroLinkedAnimLayer::GetHeroLocomotionAngle() const
{
	return CachedHeroLocomotionAngle;
}

float ULastFPSHeroLinkedAnimLayer::GetHeroDisplacementSinceLastUpdate() const
{
	return CachedHeroDisplacementSinceLastUpdate;
}

float ULastFPSHeroLinkedAnimLayer::GetHeroDisplacementSpeed() const
{
	return CachedHeroDisplacementSpeed;
}

bool ULastFPSHeroLinkedAnimLayer::HeroHasAcceleration() const
{
	return bCachedHeroHasAcceleration;
}

bool ULastFPSHeroLinkedAnimLayer::HeroHasVelocity() const
{
	return bCachedHeroHasVelocity;
}

bool ULastFPSHeroLinkedAnimLayer::ShouldHeroStop() const
{
	return bCachedHeroShouldStop;
}

bool ULastFPSHeroLinkedAnimLayer::HeroHasGroundDistance() const
{
	return bCachedHeroHasGroundDistance;
}

float ULastFPSHeroLinkedAnimLayer::GetHeroDistanceToStop() const
{
	return CachedHeroDistanceToStop;
}

float ULastFPSHeroLinkedAnimLayer::GetHeroGroundDistance() const
{
	return CachedHeroGroundDistance;
}

float ULastFPSHeroLinkedAnimLayer::GetHeroGroundFriction() const
{
	return CachedHeroGroundFriction;
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetIdleAnimation() const
{
	return GetSequence(GetActiveLocomotionSequences().Idle);
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetWalkStartAnimation(EMMCardinalDirection Direction) const
{
	return SelectDirectionalSequence(GetActiveLocomotionSequences().WalkStart, Direction, TEXT("WalkStart"));
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetWalkLoopAnimation(EMMCardinalDirection Direction) const
{
	return SelectDirectionalSequence(GetActiveLocomotionSequences().WalkLoop, Direction, TEXT("WalkLoop"));
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetWalkStopAnimation(EMMCardinalDirection Direction) const
{
	return SelectDirectionalSequence(GetActiveLocomotionSequences().WalkStop, Direction, TEXT("WalkStop"));
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetJogStartAnimation(EMMCardinalDirection Direction) const
{
	return SelectDirectionalSequence(GetActiveLocomotionSequences().JogStart, Direction, TEXT("JogStart"));
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetJogLoopAnimation(EMMCardinalDirection Direction) const
{
	return SelectDirectionalSequence(GetActiveLocomotionSequences().JogLoop, Direction, TEXT("JogLoop"));
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetJogStopAnimation(EMMCardinalDirection Direction) const
{
	return SelectDirectionalSequence(GetActiveLocomotionSequences().JogStop, Direction, TEXT("JogStop"));
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetSprintLoopAnimation() const
{
	return GetSequence(GetActiveLocomotionSequences().SprintLoop);
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetJumpStartAnimation() const
{
	return GetSequence(GetActiveLocomotionSequences().JumpStart);
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetJumpStartLoopAnimation() const
{
	return GetSequence(GetActiveLocomotionSequences().JumpStartLoop);
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetJumpApexAnimation() const
{
	return GetSequence(GetActiveLocomotionSequences().JumpApex);
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetJumpFallLoopAnimation() const
{
	return GetSequence(GetActiveLocomotionSequences().JumpFallLoop);
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetJumpFallLandAnimation() const
{
	return GetSequence(GetActiveLocomotionSequences().JumpFallLand);
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::SelectDirectionalSequence(
	const FLastFPSDirectionalSequenceSet& SequenceSet,
	EMMCardinalDirection Direction,
	const TCHAR* ContextName) const
{
	const TObjectPtr<UAnimSequenceBase>* SelectedSequence = &SequenceSet.Forward;
	bool bUsedFallback = false;
	switch (Direction)
	{
	case EMMCardinalDirection::Right:
		bUsedFallback = !SequenceSet.Right;
		SelectedSequence = SequenceSet.Right ? &SequenceSet.Right : &SequenceSet.Forward;
		break;
	case EMMCardinalDirection::Back:
		bUsedFallback = !SequenceSet.Back;
		SelectedSequence = SequenceSet.Back ? &SequenceSet.Back : &SequenceSet.Forward;
		break;
	case EMMCardinalDirection::Left:
		bUsedFallback = !SequenceSet.Left;
		SelectedSequence = SequenceSet.Left ? &SequenceSet.Left : &SequenceSet.Forward;
		break;
	case EMMCardinalDirection::Forward:
	default:
		SelectedSequence = &SequenceSet.Forward;
		break;
	}

	UAnimSequenceBase* Sequence = GetSequence(*SelectedSequence);
	if (bDebugSequenceSelection)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Linked Anim Sequence Selection | Context=%s | Requested=%s | Selected=%s | Fallback=%s | Fwd=%s | Right=%s | Back=%s | Left=%s"),
			ContextName,
			*GetLinkedLayerCardinalDirectionName(Direction),
			*GetNameSafe(Sequence),
			bUsedFallback ? TEXT("true") : TEXT("false"),
			*GetNameSafe(SequenceSet.Forward.Get()),
			*GetNameSafe(SequenceSet.Right.Get()),
			*GetNameSafe(SequenceSet.Back.Get()),
			*GetNameSafe(SequenceSet.Left.Get()));
	}

	return Sequence;
}

UAnimSequenceBase* ULastFPSHeroLinkedAnimLayer::GetSequence(
	const TObjectPtr<UAnimSequenceBase>& Sequence) const
{
	return Sequence.Get();
}

const FLastFPSHeroLinkedLocomotionSequences& ULastFPSHeroLinkedAnimLayer::GetActiveLocomotionSequences() const
{
	return LocomotionSet ? LocomotionSet->LocomotionSequences : LocomotionSequences;
}
