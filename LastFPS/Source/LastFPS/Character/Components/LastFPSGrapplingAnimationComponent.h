#pragma once

#include "CoreMinimal.h"
#include "Character/Animation/LastFPSGrapplingAnimationTypes.h"
#include "Components/ActorComponent.h"
#include "LastFPSGrapplingAnimationComponent.generated.h"

/** 그래플링 GA의 상태를 팔 IK용 컴포넌트 공간 값으로 변환한다. */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSGrapplingAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastFPSGrapplingAnimationComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
	bool IsGrapplingIKActive() const
	{
		return RuntimeState.Phase == ELastFPSGrapplingAnimationPhase::Pulling;
	}

	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
	ELastFPSGrapplingAnimationPhase GetGrapplingAnimationPhase() const { return RuntimeState.Phase; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
	float GetHookFlightDuration() const { return RuntimeState.HookFlightDuration; }

	/** Animation Blueprint의 Two Bone IK Effector Location에 연결할 메시 컴포넌트 공간 값이다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
	FVector GetGrapplingIKEffectorLocation() const { return GrapplingIKEffectorLocation; }

	/** Animation Blueprint의 Two Bone IK Joint Target Location에 연결할 메시 컴포넌트 공간 값이다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
	FVector GetGrapplingIKJointTargetLocation() const { return GrapplingIKJointTargetLocation; }

	/** Transform (Modify) Bone의 Component Space Rotation에 연결할 값이다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
	FRotator GetGrapplingIKHandRotation() const { return GrapplingIKHandRotation; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
	float GetGrapplingIKAlpha() const { return GrapplingIKAlpha; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
	float GetGrapplingBodyPitch() const { return GrapplingBodyPitch; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
	FVector GetGrappleAnchorWorldLocation() const
	{
		return FVector(RuntimeState.AnchorWorldLocation);
	}

	/** 요청 소유권을 기록해 이전 Ability가 최신 그래플링 상태를 지우지 못하게 한다. */
	void BeginHookFlight(
		UObject* RequestOwner,
		const FVector& AnchorWorldLocation,
		float HookFlightDuration);
	void BeginPulling(
		UObject* RequestOwner,
		const FVector& AnchorWorldLocation,
		const FLastFPSGrapplingIKSettings& IKSettings);
	void EndGrappling(UObject* RequestOwner);

private:
	bool CanModifyPredictedState() const;
	void ApplyRuntimeState(const FLastFPSReplicatedGrapplingAnimationState& NewState);
	void UpdateIKTargets();
	void UpdateIKAlpha(float DeltaTime);

	UFUNCTION()
	void OnRep_ReplicatedState();

	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedState)
	FLastFPSReplicatedGrapplingAnimationState ReplicatedState;

	UPROPERTY(Transient, BlueprintReadOnly, Category="LastFPS|Grappling Animation", meta=(AllowPrivateAccess="true"))
	FVector GrapplingIKEffectorLocation = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category="LastFPS|Grappling Animation", meta=(AllowPrivateAccess="true"))
	FVector GrapplingIKJointTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category="LastFPS|Grappling Animation", meta=(AllowPrivateAccess="true"))
	FRotator GrapplingIKHandRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, BlueprintReadOnly, Category="LastFPS|Grappling Animation", meta=(AllowPrivateAccess="true"))
	float GrapplingIKAlpha = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="LastFPS|Grappling Animation", meta=(AllowPrivateAccess="true"))
	float GrapplingBodyPitch = 0.f;

	float GrapplingBodyPitchTarget = 0.f;
	FLastFPSReplicatedGrapplingAnimationState RuntimeState;
	TWeakObjectPtr<UObject> ActiveRequestOwner;
	bool bLoggedInvalidShoulderBone = false;
};
