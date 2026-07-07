#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Utility/LastFPSEnumTypes.h"
#include "LastFPSBaseAnimInstance.generated.h"

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

protected:
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
