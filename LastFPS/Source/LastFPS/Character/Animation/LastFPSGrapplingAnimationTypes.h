#pragma once

#include "CoreMinimal.h"
#include "LastFPSGrapplingAnimationTypes.generated.h"

/** 그래플링 애니메이션 상태 머신이 사용하는 진행 단계입니다. */
UENUM(BlueprintType)
enum class ELastFPSGrapplingAnimationPhase : uint8
{
	None,
	HookFlight,
	Pulling
};

/** 그래플링 팔 IK를 계산하는 데 필요한 불변 설정이다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSGrapplingIKSettings
{
	GENERATED_BODY()

	/** Effector의 기준점으로 사용할 팔 시작 뼈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling IK")
	FName ShoulderBoneName = TEXT("upperarm_r");

	/** 어깨에서 훅 방향으로 손을 배치할 거리다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling IK", meta=(ClampMin="1.0", Units="cm"))
	float HandReachDistance = 60.f;

	/** 계산된 Effector에 메시 컴포넌트 공간으로 더하는 보정값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling IK", meta=(Units="cm"))
	FVector EffectorOffset = FVector::ZeroVector;

	/** 어깨를 기준으로 한 팔꿈치 목표의 메시 컴포넌트 공간 보정값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling IK", meta=(Units="cm"))
	FVector JointTargetOffset = FVector(-15.f, 40.f, 0.f);

	/** 손 뼈의 실제 전방 축과 훅 방향의 차이를 보정하는 로컬 회전값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling IK")
	FRotator HandRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling IK", meta=(ClampMin="0.0", Units="s"))
	float BlendInDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling IK", meta=(ClampMin="0.0", Units="s"))
	float BlendOutDuration = 0.1f;

	/** 훅 고저각을 몸체 기울기로 변환하는 배율입니다. 방향이 반대면 음수로 설정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Body")
	float BodyPitchScale = -1.f;

	/** 몸체가 과도하게 뒤집히지 않도록 제한하는 최대 기울기입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Body", meta=(ClampMin="0.0", ClampMax="89.0", Units="deg"))
	float MaximumBodyPitchAngle = 70.f;

	/** 목표 몸체 기울기에 접근하는 보간 속도입니다. 값이 높을수록 빠르게 반응합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Body", meta=(ClampMin="0.01"))
	float BodyPitchInterpSpeed = 8.f;

	/** 몽타주 자체의 기준 자세와 훅 방향 사이를 맞추는 추가 기울기입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Body", meta=(Units="deg"))
	float BodyPitchOffset = 0.f;
};

/** 그래플링 애니메이션 상태를 한 번의 복제로 전달하는 데이터 계약이다. */
USTRUCT()
struct FLastFPSReplicatedGrapplingAnimationState
{
	GENERATED_BODY()

	UPROPERTY()
	ELastFPSGrapplingAnimationPhase Phase = ELastFPSGrapplingAnimationPhase::None;

	UPROPERTY()
	float HookFlightDuration = 0.f;

	UPROPERTY()
	FVector_NetQuantize10 AnchorWorldLocation = FVector::ZeroVector;

	UPROPERTY()
	FLastFPSGrapplingIKSettings IKSettings;

	/** 그래플링 시작 프레임에 계산한 몸체 기울기입니다. */
	UPROPERTY()
	float InitialBodyPitch = 0.f;
};
