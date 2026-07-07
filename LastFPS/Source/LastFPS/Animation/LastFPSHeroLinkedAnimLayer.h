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

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	TObjectPtr<ULastFPSLocomotionAnimationSet> LocomotionSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|LinkedLayer|Locomotion")
	FLastFPSHeroLinkedLocomotionSequences LocomotionSequences;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|LinkedLayer|Debug")
	bool bDebugSequenceSelection = true;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer", meta=(BlueprintThreadSafe))
	ULastFPSAnimInstance* GetHeroAnimInstance() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	EMMLocomotionState GetHeroLocomotionState() const;

	UFUNCTION(BlueprintPure, Category="MM|LinkedLayer|Locomotion", meta=(BlueprintThreadSafe))
	EMMCardinalDirection GetHeroStartCardinalDirection() const;

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
