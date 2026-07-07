#pragma once

#include "CoreMinimal.h"
#include "Animation/LastFPSBaseAnimInstance.h"
#include "Animation/LastFPSLocomotionAnimationSet.h"
#include "LastFPSHeroLinkedAnimLayer.generated.h"

class ULastFPSAnimInstance;
class UAnimSequenceBase;

UCLASS(Abstract, Blueprintable)
class LASTFPS_API ULastFPSHeroLinkedAnimLayer : public ULastFPSBaseAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	TObjectPtr<ULastFPSLocomotionAnimationSet> LocomotionSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	FLastFPSHeroLinkedLocomotionSequences LocomotionSequences;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|LinkedLayer|Debug")
	bool bDebugSequenceSelection = true;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	EMMLocomotionState CachedHeroLocomotionState = EMMLocomotionState::Idle;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	EMMCardinalDirection CachedHeroStartCardinalDirection = EMMCardinalDirection::Forward;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	EMMCardinalDirection CachedHeroStopCardinalDirection = EMMCardinalDirection::Forward;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	EMMCardinalDirection CachedHeroLocomotionCardinalDirection = EMMCardinalDirection::Forward;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	FVector CachedHeroVelocity2D = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	float CachedHeroLocomotionAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	float CachedHeroDisplacementSinceLastUpdate = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	float CachedHeroDisplacementSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	bool bCachedHeroHasAcceleration = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	bool bCachedHeroHasVelocity = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|DistanceMatching")
	bool bCachedHeroShouldStop = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|DistanceMatching")
	bool bCachedHeroHasGroundDistance = false;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|DistanceMatching")
	float CachedHeroDistanceToStop = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|DistanceMatching")
	float CachedHeroGroundDistance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MM|LinkedLayer|Movement")
	float CachedHeroGroundFriction = 0.f;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer")
	ULastFPSAnimInstance* GetHeroAnimInstance() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	EMMLocomotionState GetHeroLocomotionState() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	EMMCardinalDirection GetHeroStartCardinalDirection() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	EMMCardinalDirection GetHeroStopCardinalDirection() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	EMMCardinalDirection GetHeroLocomotionCardinalDirection() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	FVector GetHeroVelocity2D() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	float GetHeroLocomotionAngle() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	float GetHeroDisplacementSinceLastUpdate() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	float GetHeroDisplacementSpeed() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	bool HeroHasAcceleration() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	bool HeroHasVelocity() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|DistanceMatching", meta=(BlueprintThreadSafe))
	bool ShouldHeroStop() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|DistanceMatching", meta=(BlueprintThreadSafe))
	bool HeroHasGroundDistance() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|DistanceMatching", meta=(BlueprintThreadSafe))
	float GetHeroDistanceToStop() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|DistanceMatching", meta=(BlueprintThreadSafe))
	float GetHeroGroundDistance() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Movement", meta=(BlueprintThreadSafe))
	float GetHeroGroundFriction() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetIdleAnimation() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetWalkStartAnimation(EMMCardinalDirection Direction) const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetWalkLoopAnimation(EMMCardinalDirection Direction) const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetWalkStopAnimation(EMMCardinalDirection Direction) const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetJogStartAnimation(EMMCardinalDirection Direction) const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetJogLoopAnimation(EMMCardinalDirection Direction) const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetJogStopAnimation(EMMCardinalDirection Direction) const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetSprintLoopAnimation() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetJumpStartAnimation() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetJumpStartLoopAnimation() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetJumpApexAnimation() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetJumpFallLoopAnimation() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequenceBase* GetJumpFallLandAnimation() const;

	UAnimSequenceBase* SelectDirectionalSequence(
		const FLastFPSDirectionalSequenceSet& SequenceSet,
		EMMCardinalDirection Direction,
		const TCHAR* ContextName) const;

	UAnimSequenceBase* GetSequence(
		const TObjectPtr<UAnimSequenceBase>& Sequence) const;

	const FLastFPSHeroLinkedLocomotionSequences& GetActiveLocomotionSequences() const;
};
