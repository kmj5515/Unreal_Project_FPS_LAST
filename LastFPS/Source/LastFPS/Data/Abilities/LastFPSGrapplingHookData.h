#pragma once

#include "CoreMinimal.h"
#include "Character/Animation/LastFPSGrapplingAnimationTypes.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "LastFPSGrapplingHookData.generated.h"

class UCurveFloat;
class UCurveVector;

UENUM(BlueprintType)
enum class ELastFPSGrappleFinishVelocityMode : uint8
{
	Maintain,
	Stop,
	Clamp
};

/** 그래플링 훅의 판정, 이동 및 연출 설정을 보관하는 불변 데이터다. */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSGrapplingHookData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULastFPSGrapplingHookData();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Targeting", meta=(ClampMin="1.0", Units="cm"))
	float MaximumDistance = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Targeting", meta=(ClampMin="0.0", Units="cm"))
	float MinimumDistance = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Targeting")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** 활성화하면 움직이는 Actor에는 와이어를 걸 수 없다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Targeting")
	bool bRequireStaticSurface = true;

	/** 로컬 HUD용 타깃 미리보기 검사 간격이다. 실제 사용 시에는 서버에서 다시 검증한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Targeting", meta=(ClampMin="0.02", Units="s"))
	float TargetPreviewRefreshInterval = 0.05f;

	/** 고정점을 표면 바깥쪽으로 이동해 와이어 끝이 지형 안에 묻히는 것을 방지한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Targeting", meta=(ClampMin="0.0", Units="cm"))
	float AnchorSurfaceOffset = 3.f;

	/** 훅이 발사된 뒤 표면에 부착되기까지의 연출 시간입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Flight", meta=(ClampMin="0.0", Units="s"))
	float HookFlightDuration = 0.18f;

	/** 훅 부착 완료 후 플레이어 당김을 시작하기 전까지 유지하는 시간입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Flight", meta=(ClampMin="0.0", Units="s"))
	float AttachDelayDuration = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Movement", meta=(ClampMin="1.0", Units="cm/s"))
	float PullSpeed = 1800.f;

	/** 캐릭터 캡슐이 지형에 충돌하기 전에 멈추도록 고정점 앞에 남기는 거리다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Movement", meta=(ClampMin="0.0", Units="cm"))
	float StopDistance = 0.f;

	/** 고정점보다 위로 올라가도록 최종 도착 위치에 더하는 월드 Z 높이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Movement", meta=(ClampMin="0.0", Units="cm"))
	float ArrivalHeightOffset = 140.f;

	/** 벽에 막히지 않도록 최종 도착 위치를 피격 표면 바깥쪽으로 밀어내는 거리다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Movement", meta=(ClampMin="0.0", Units="cm"))
	float ArrivalSurfaceClearance = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Movement")
	bool bRestrictSpeedToExpected = true;

	/** 비어 있으면 직선 경로를 사용하며, 지정하면 진행 방향 기준의 경로 오프셋으로 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Movement")
	TObjectPtr<UCurveVector> PathOffsetCurve;

	/** 실제 시간 0~1을 이동 진행률 0~1로 변환해 가속과 감속을 제어한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Movement")
	TSoftObjectPtr<UCurveFloat> MovementProgressCurve = TSoftObjectPtr<UCurveFloat>(
		FSoftObjectPath(TEXT("/Game/Data/Curves/CF_GrappleMovementProgress.CF_GrappleMovementProgress")));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Movement")
	ELastFPSGrappleFinishVelocityMode FinishVelocityMode = ELastFPSGrappleFinishVelocityMode::Clamp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Movement", meta=(ClampMin="0.0", Units="cm/s", EditCondition="FinishVelocityMode == ELastFPSGrappleFinishVelocityMode::Clamp"))
	float FinishVelocityClamp = 1200.f;

	/** 런타임 팔 IK는 이 설정과 훅 위치만 소비하며 구체적인 Animation Blueprint를 알지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Animation|IK")
	FLastFPSGrapplingIKSettings IKSettings;

	/** 당김 중 카메라가 캐릭터를 따라오는 속도입니다. 값이 낮을수록 더 느리게 따라옵니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Camera", meta=(ClampMin="0.01"))
	float PullCameraLagSpeed = 4.f;

	/** 당김 중 로컬 카메라에 적용할 모션 블러 강도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Camera", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PullMotionBlurAmount = 0.35f;

	/** 당김 종료 후 카메라 랙과 블러가 원래 값으로 돌아가는 시간입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Camera", meta=(ClampMin="0.0", Units="s"))
	float PullCameraBlendOutDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Gameplay Cue", meta=(Categories="GameplayCue"))
	FGameplayTag WireGameplayCueTag;

	/** Gameplay Cue가 캐릭터 메시에서 와이어 시작점을 찾을 때 사용하는 소켓이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Gameplay Cue")
	FName WireStartSocketName = NAME_None;

	/** 선택한 소켓의 로컬 공간에 적용하는 와이어 시작점 보정값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grappling Hook|Gameplay Cue", meta=(Units="cm"))
	FVector WireStartOffset = FVector::ZeroVector;
};
