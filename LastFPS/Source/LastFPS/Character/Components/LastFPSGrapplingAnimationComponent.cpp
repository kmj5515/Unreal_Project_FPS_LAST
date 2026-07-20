#include "Character/Components/LastFPSGrapplingAnimationComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSGrapplingAnimation, Log, All);

ULastFPSGrapplingAnimationComponent::ULastFPSGrapplingAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(true);
}

void ULastFPSGrapplingAnimationComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULastFPSGrapplingAnimationComponent, ReplicatedState);
}

void ULastFPSGrapplingAnimationComponent::BeginHookFlight(
	UObject* RequestOwner,
	const FVector& AnchorWorldLocation,
	const float HookFlightDuration)
{
	AActor* OwnerActor = GetOwner();
	if (!CanModifyPredictedState()
		|| !IsValid(RequestOwner)
		|| AnchorWorldLocation.ContainsNaN()
		|| !FMath::IsFinite(HookFlightDuration))
	{
		UE_LOG(LogLastFPSGrapplingAnimation, Error,
			TEXT("그래플링 훅 비행 상태 시작 실패: Owner=%s, RequestOwner=%s, Anchor=%s, Duration=%.3f"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(RequestOwner),
			*AnchorWorldLocation.ToCompactString(),
			HookFlightDuration);
		return;
	}

	FLastFPSReplicatedGrapplingAnimationState NewState;
	NewState.Phase = ELastFPSGrapplingAnimationPhase::HookFlight;
	NewState.AnchorWorldLocation = AnchorWorldLocation;
	NewState.HookFlightDuration = FMath::Max(0.f, HookFlightDuration);
	ActiveRequestOwner = RequestOwner;
	ApplyRuntimeState(NewState);

	if (OwnerActor && OwnerActor->HasAuthority())
	{
		ReplicatedState = NewState;
		OwnerActor->ForceNetUpdate();
	}
}

void ULastFPSGrapplingAnimationComponent::BeginPulling(
	UObject* RequestOwner,
	const FVector& AnchorWorldLocation,
	const FLastFPSGrapplingIKSettings& IKSettings)
{
	AActor* OwnerActor = GetOwner();
	if (!CanModifyPredictedState() || !IsValid(RequestOwner) || AnchorWorldLocation.ContainsNaN())
	{
		UE_LOG(LogLastFPSGrapplingAnimation, Error,
			TEXT("그래플링 애니메이션 시작 실패: Owner=%s, RequestOwner=%s, Anchor=%s, 원인=권한 또는 입력이 유효하지 않습니다."),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(RequestOwner),
			*AnchorWorldLocation.ToCompactString());
		return;
	}

	FLastFPSReplicatedGrapplingAnimationState NewState;
	NewState.Phase = ELastFPSGrapplingAnimationPhase::Pulling;
	NewState.HookFlightDuration = RuntimeState.HookFlightDuration;
	NewState.AnchorWorldLocation = AnchorWorldLocation;
	NewState.IKSettings = IKSettings;
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
	{
		const FVector InitialDirection = (
			AnchorWorldLocation - OwnerCharacter->GetActorLocation()).GetSafeNormal();
		if (!InitialDirection.IsNearlyZero())
		{
			const float MaximumPitch = FMath::Max(0.f, IKSettings.MaximumBodyPitchAngle);
			NewState.InitialBodyPitch = FMath::Clamp(
				InitialDirection.Rotation().Pitch * IKSettings.BodyPitchScale
					+ IKSettings.BodyPitchOffset,
				-MaximumPitch,
				MaximumPitch);
		}
	}
	ActiveRequestOwner = RequestOwner;
	ApplyRuntimeState(NewState);

	if (OwnerActor && OwnerActor->HasAuthority())
	{
		ReplicatedState = NewState;
		OwnerActor->ForceNetUpdate();
	}
}

void ULastFPSGrapplingAnimationComponent::EndGrappling(UObject* RequestOwner)
{
	AActor* OwnerActor = GetOwner();
	if (!CanModifyPredictedState())
	{
		return;
	}

	if (ActiveRequestOwner.IsValid() && ActiveRequestOwner.Get() != RequestOwner)
	{
		return;
	}

	ActiveRequestOwner.Reset();
	FLastFPSReplicatedGrapplingAnimationState NewState = RuntimeState;
	NewState.Phase = ELastFPSGrapplingAnimationPhase::None;
	ApplyRuntimeState(NewState);

	if (OwnerActor && OwnerActor->HasAuthority())
	{
		ReplicatedState = NewState;
		OwnerActor->ForceNetUpdate();
	}
}

bool ULastFPSGrapplingAnimationComponent::CanModifyPredictedState() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	if (OwnerActor->HasAuthority())
	{
		return true;
	}

	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

void ULastFPSGrapplingAnimationComponent::ApplyRuntimeState(
	const FLastFPSReplicatedGrapplingAnimationState& NewState)
{
	RuntimeState = NewState;
	GrapplingBodyPitchTarget = RuntimeState.InitialBodyPitch;
	bLoggedInvalidShoulderBone = false;
	SetComponentTickEnabled(true);
	UpdateIKTargets();
	UpdateIKAlpha(0.f);
}

void ULastFPSGrapplingAnimationComponent::OnRep_ReplicatedState()
{
	ApplyRuntimeState(ReplicatedState);
}

void ULastFPSGrapplingAnimationComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateIKTargets();
	UpdateIKAlpha(DeltaTime);
}

void ULastFPSGrapplingAnimationComponent::UpdateIKTargets()
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* MeshComponent = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	if (!MeshComponent || !MeshComponent->DoesSocketExist(RuntimeState.IKSettings.ShoulderBoneName))
	{
		if (!bLoggedInvalidShoulderBone)
		{
			UE_LOG(LogLastFPSGrapplingAnimation, Error,
				TEXT("그래플링 IK 계산 실패: Owner=%s, ShoulderBone=%s, 원인=메시 또는 기준 뼈를 찾을 수 없습니다."),
				*GetNameSafe(OwnerCharacter),
				*RuntimeState.IKSettings.ShoulderBoneName.ToString());
			bLoggedInvalidShoulderBone = true;
		}
		return;
	}

	const FVector ShoulderLocation = MeshComponent->GetSocketTransform(
		RuntimeState.IKSettings.ShoulderBoneName,
		RTS_Component).GetLocation();
	const FVector AnchorComponentLocation = MeshComponent->GetComponentTransform().InverseTransformPosition(
		FVector(RuntimeState.AnchorWorldLocation));
	const FVector DirectionToAnchor = (AnchorComponentLocation - ShoulderLocation).GetSafeNormal();
	if (DirectionToAnchor.IsNearlyZero())
	{
		return;
	}

	GrapplingIKEffectorLocation = ShoulderLocation
		+ DirectionToAnchor * FMath::Max(1.f, RuntimeState.IKSettings.HandReachDistance)
		+ RuntimeState.IKSettings.EffectorOffset;
	GrapplingIKJointTargetLocation = ShoulderLocation
		+ RuntimeState.IKSettings.JointTargetOffset;
	const FQuat AimRotation = DirectionToAnchor.ToOrientationQuat();
	GrapplingIKHandRotation = (AimRotation
		* RuntimeState.IKSettings.HandRotationOffset.Quaternion()).Rotator();
}

void ULastFPSGrapplingAnimationComponent::UpdateIKAlpha(const float DeltaTime)
{
	const bool bIsPulling = RuntimeState.Phase == ELastFPSGrapplingAnimationPhase::Pulling;
	const float TargetAlpha = bIsPulling ? 1.f : 0.f;
	const float BlendDuration = bIsPulling
		? RuntimeState.IKSettings.BlendInDuration
		: RuntimeState.IKSettings.BlendOutDuration;

	if (BlendDuration <= KINDA_SMALL_NUMBER)
	{
		GrapplingIKAlpha = TargetAlpha;
	}
	else if (DeltaTime > 0.f)
	{
		GrapplingIKAlpha = FMath::FInterpConstantTo(
			GrapplingIKAlpha,
			TargetAlpha,
			DeltaTime,
			1.f / BlendDuration);
	}
	if (DeltaTime > 0.f)
	{
		const float DesiredBodyPitch = bIsPulling ? GrapplingBodyPitchTarget : 0.f;
		GrapplingBodyPitch = FMath::FInterpTo(
			GrapplingBodyPitch,
			DesiredBodyPitch,
			DeltaTime,
			FMath::Max(0.01f, RuntimeState.IKSettings.BodyPitchInterpSpeed));
	}

	if (!bIsPulling
		&& GrapplingIKAlpha <= KINDA_SMALL_NUMBER
		&& FMath::IsNearlyZero(GrapplingBodyPitch, 0.1f))
	{
		GrapplingIKAlpha = 0.f;
		GrapplingBodyPitch = 0.f;
		SetComponentTickEnabled(false);
	}
}
