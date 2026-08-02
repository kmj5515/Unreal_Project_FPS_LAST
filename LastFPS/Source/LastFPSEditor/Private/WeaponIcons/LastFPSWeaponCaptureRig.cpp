#include "WeaponIcons/LastFPSWeaponCaptureRig.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSWeaponCaptureRig, Log, All);

namespace
{
	/** Intensity 를 저작한 기준 거리(cm). 실제 거리가 달라지면 이 값을 기준으로 광량을 보정한다. */
	constexpr float LightReferenceDistance = 100.f;

	/** 조명이 피사체 안으로 들어가지 않도록 두는 하한. */
	constexpr float MinimumLightDistance = 1.f;

	/** 감쇠가 잘려 어두워지지 않도록 조명 거리에 곱하는 감쇠 반경 배수. */
	constexpr float AttenuationRadiusScale = 4.f;

	/** 바운드 반경 대비 광원 크기. 클수록 하이라이트와 그림자 경계가 부드러워진다. */
	constexpr float SourceRadiusScale = 0.5f;
}

ALastFPSWeaponCaptureRig::ALastFPSWeaponCaptureRig()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetActorEnableCollision(false);

	RigRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RigRoot"));
	SetRootComponent(RigRoot);
	RigRoot->SetMobility(EComponentMobility::Movable);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RigRoot);
	WeaponMesh->SetMobility(EComponentMobility::Movable);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 전용 라이팅 채널로 격리한다. ShowOnly 목록은 프리미티브만 거르고 레벨 조명은 거르지 못한다.
	WeaponMesh->SetLightingChannels(false, false, true);
	WeaponMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// 배치와 세기는 무엇을 찍는지 알아야 정할 수 있으므로 ConfigureLighting 으로 미룬다.
	// 생성자에서는 피사체와 무관한 성질만 고정한다.
	const auto CreateStudioLight = [this](const TCHAR* Name) -> UPointLightComponent*
	{
		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(Name);
		Light->SetupAttachment(RigRoot);
		Light->SetMobility(EComponentMobility::Movable);
		// 루멘 단위의 광량이 거리에 따라 감쇠되어 머티리얼 색이 하얗게 클리핑되지 않게 한다.
		Light->SetUseInverseSquaredFalloff(true);
		Light->SetLightingChannels(false, false, true);
		return Light;
	};

	KeyLight = CreateStudioLight(TEXT("KeyLight"));
	FillLight = CreateStudioLight(TEXT("FillLight"));
	RimLight = CreateStudioLight(TEXT("RimLight"));
}

void ALastFPSWeaponCaptureRig::ConfigureLighting(
	const FLastFPSIconLightSetup& KeySetup,
	const FLastFPSIconLightSetup& FillSetup,
	const FLastFPSIconLightSetup& RimSetup,
	const FQuat& CameraRotation,
	const float BoundsRadius)
{
	ApplyLightSetup(KeyLight, KeySetup, CameraRotation, BoundsRadius);
	ApplyLightSetup(FillLight, FillSetup, CameraRotation, BoundsRadius);
	ApplyLightSetup(RimLight, RimSetup, CameraRotation, BoundsRadius);
}

void ALastFPSWeaponCaptureRig::ApplyLightSetup(
	UPointLightComponent* Light,
	const FLastFPSIconLightSetup& Setup,
	const FQuat& CameraRotation,
	const float BoundsRadius)
{
	if (!Light)
	{
		return;
	}

	const FVector LocalDirection = Setup.Direction.GetSafeNormal();
	if (LocalDirection.IsNearlyZero())
	{
		// 방향이 없으면 조명이 피사체 안에 박혀 결과를 예측할 수 없다. 조용히 넘기지 않는다.
		UE_LOG(
			LogLastFPSWeaponCaptureRig,
			Warning,
			TEXT("아이콘 조명 배치를 건너뜁니다: 조명='%s', 원인=Direction 이 영벡터입니다."),
			*Light->GetName());
		return;
	}

	const float Distance = FMath::Max(BoundsRadius * Setup.DistanceScale, MinimumLightDistance);

	Light->SetRelativeLocation(CameraRotation.RotateVector(LocalDirection) * Distance);
	Light->SetLightColor(Setup.Color);
	// 역제곱 감쇠라 거리가 멀어지면 그만큼 어두워진다. 무기 크기가 달라도 같은 노출을 유지하려면
	// 기준 거리 대비 제곱만큼 광량을 올려 줘야 한다.
	Light->SetIntensity(Setup.Intensity * FMath::Square(Distance / LightReferenceDistance));
	Light->SetAttenuationRadius(Distance * AttenuationRadiusScale);
	Light->SetSourceRadius(FMath::Max(BoundsRadius * SourceRadiusScale, MinimumLightDistance));
	Light->SetCastShadows(Setup.bCastShadows);
}
