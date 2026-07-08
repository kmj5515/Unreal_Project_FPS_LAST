#include "Animation/LastFPSBaseAnimInstance.h"

EMMCardinalDirection ULastFPSBaseAnimInstance::DirectionToCardinalDirection(float InDirection) const
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

EMMCardinalDirection ULastFPSBaseAnimInstance::DirectionEMM(float InDirection) const
{
	// 1. 방향값을 -180 ~ 180 범위로 정규화 (언리얼 표준 회전값 기준)
	const float NormalizedDir = FRotator::NormalizeAxis(InDirection);

	// 2. Forward 범위 체크 (이미지의 첫 번째 Branch)
	if (NormalizedDir >= -45.0f && NormalizedDir <= 45.0f)
	{
		return EMMCardinalDirection::Forward;
	}

	// 3. Backward 범위 체크 (이미지의 두 번째 Branch)
	// 언리얼에서 Backward는 보통 135~180 또는 -180~-135 양끝으로 나뉘므로 OR 연산이나 절대값을 사용합니다.
	const bool bIsBackward = (NormalizedDir <= -135.0f) || (NormalizedDir >= 135.0f);
	if (bIsBackward)
	{
		return EMMCardinalDirection::Back;
	}

	// 4. Left / Right 체크 (이미지의 세 번째 Branch: 0보다 작으면 Left, 아니면 Right)
	if (NormalizedDir < 0.0f)
	{
		return EMMCardinalDirection::Left;
	}
	else
	{
		return EMMCardinalDirection::Right;
	}
}

EMMCardinalDirection ULastFPSBaseAnimInstance::DirectionToStableCardinalDirection(
	float InDirection,
	EMMCardinalDirection CurrentDirection,
	float Hysteresis) const
{
	const float CurrentDirectionAngle = CardinalDirectionToAngle(CurrentDirection);
	const float DeltaFromCurrent = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentDirectionAngle, InDirection));
	const float KeepCurrentThreshold = 45.f + Hysteresis;

	if (DeltaFromCurrent <= KeepCurrentThreshold)
	{
		return CurrentDirection;
	}

	return DirectionToCardinalDirection(InDirection);
}

EMMCardinalDirection ULastFPSBaseAnimInstance::CalculateLocomotionDirection(
	float CurrentLocomotionAngle,
	float BackwardMin,
	float BackwardMax,
	float ForwardMin,
	float ForwardMax,
	EMMCardinalDirection CurrentDirection,
	float DeadZone)
{
	const float Angle = FRotator::NormalizeAxis(CurrentLocomotionAngle);
	const float ClampedDeadZone = FMath::Max(0.f, DeadZone);

	const auto IsForward = [Angle](float Min, float Max)
	{
		return Angle >= Min && Angle <= Max;
	};

	const auto IsBackward = [Angle](float Min, float Max)
	{
		return Angle <= Min || Angle >= Max;
	};

	const auto IsLeft = [Angle](float BackMin, float FwdMin)
	{
		return Angle > BackMin && Angle < FwdMin;
	};

	const auto IsRight = [Angle](float FwdMax, float BackMax)
	{
		return Angle > FwdMax && Angle < BackMax;
	};

	switch (CurrentDirection)
	{
	case EMMCardinalDirection::Forward:
		if (IsForward(ForwardMin - ClampedDeadZone, ForwardMax + ClampedDeadZone))
		{
			return CurrentDirection;
		}
		break;
	case EMMCardinalDirection::Back:
		if (IsBackward(BackwardMin + ClampedDeadZone, BackwardMax - ClampedDeadZone))
		{
			return CurrentDirection;
		}
		break;
	case EMMCardinalDirection::Left:
		if (IsLeft(BackwardMin - ClampedDeadZone, ForwardMin + ClampedDeadZone))
		{
			return CurrentDirection;
		}
		break;
	case EMMCardinalDirection::Right:
		if (IsRight(ForwardMax - ClampedDeadZone, BackwardMax + ClampedDeadZone))
		{
			return CurrentDirection;
		}
		break;
	default:
		break;
	}

	if (IsForward(ForwardMin, ForwardMax))
	{
		return EMMCardinalDirection::Forward;
	}

	if (IsBackward(BackwardMin, BackwardMax))
	{
		return EMMCardinalDirection::Back;
	}

	return Angle < 0.f
		       ? EMMCardinalDirection::Left
		       : EMMCardinalDirection::Right;
}

float ULastFPSBaseAnimInstance::CardinalDirectionToAngle(EMMCardinalDirection InDirection) const
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
