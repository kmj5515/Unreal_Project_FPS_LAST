#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Utility/LastFPSEnumTypes.h"
#include "LastFPSBaseAnimInstance.generated.h"

class ULastFPSLocomotionAnimationSet;

UCLASS(Abstract, Blueprintable)
class LASTFPS_API ULastFPSBaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="MM|Locomotion", meta=(BlueprintPure=false, BlueprintThreadSafe))
	EMMCardinalDirection CalculateLocomotionDirection(
		float CurrentLocomotionAngle,
		float BackwardMin,
		float BackwardMax,
		float ForwardMin,
		float ForwardMax,
		EMMCardinalDirection CurrentDirection,
		float DeadZone);

	/** 로코모션 애니메이션 세트(단일 소스). CharacterAnimInstance·링크드 레이어 등이 공유 참조한다. */
	UFUNCTION(BlueprintPure, Category="MM|Locomotion", meta=(BlueprintThreadSafe))
	ULastFPSLocomotionAnimationSet* GetLocomotionSet() const { return LocomotionSet; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Locomotion")
	TObjectPtr<ULastFPSLocomotionAnimationSet> LocomotionSet;

	UFUNCTION(BlueprintPure, Category="MM|Locomotion")
	EMMCardinalDirection DirectionToCardinalDirection(float InDirection) const;

	UFUNCTION(BlueprintPure, Category="MM|Locomotion")
	EMMCardinalDirection DirectionEMM(float InDirection) const;

	UFUNCTION(BlueprintPure, Category="MM|Locomotion")
	EMMCardinalDirection DirectionToStableCardinalDirection(
		float InDirection,
		EMMCardinalDirection CurrentDirection,
		float Hysteresis) const;

	UFUNCTION(BlueprintPure, Category="MM|Locomotion")
	float CardinalDirectionToAngle(EMMCardinalDirection InDirection) const;
};
