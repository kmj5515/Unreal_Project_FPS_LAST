#include "Encounter/LastFPSRoomBarrierPresentationComponent.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Encounter/LastFPSRoomEncounterSettings.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/TriggerBox.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSRoomBarrierPresentation, Log, All);

ULastFPSRoomBarrierPresentationComponent::ULastFPSRoomBarrierPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetCastShadow(false);
	SetReceivesDecals(false);
	SetTranslucentSortPriority(100);
	SetVisibility(false, true);
}

void ULastFPSRoomBarrierPresentationComponent::Configure(
	ATriggerBox& BarrierVolume,
	const FLastFPSRoomBarrierPresentationSettings& Settings)
{
	UBoxComponent* BarrierBox = BarrierVolume.FindComponentByClass<UBoxComponent>();
	if (!BarrierBox)
	{
		UE_LOG(
			LogLastFPSRoomBarrierPresentation,
			Error,
			TEXT("배리어 '%s'의 시각 표시를 구성할 수 없습니다. BoxComponent가 없습니다."),
			*BarrierVolume.GetPathName());
		return;
	}

	UStaticMesh* Mesh = Settings.Mesh.LoadSynchronous();
	UMaterialInterface* Material = Settings.Material.LoadSynchronous();
	if (!Mesh || !Material)
	{
		UE_LOG(
			LogLastFPSRoomBarrierPresentation,
			Error,
			TEXT("배리어 '%s'의 시각 에셋을 불러오지 못했습니다. Mesh=%s, Material=%s"),
			*BarrierVolume.GetPathName(),
			*Settings.Mesh.ToString(),
			*Settings.Material.ToString());
		return;
	}

	BaseWorldLocation = BarrierBox->GetComponentLocation();
	BarrierUpDirection = BarrierBox->GetUpVector();
	SetWorldLocationAndRotation(BaseWorldLocation, BarrierBox->GetComponentQuat());
	SetStaticMesh(Mesh);

	const FVector MeshSize = Mesh->GetBounds().BoxExtent * 2.0f;
	const FVector BarrierSize = BarrierBox->GetScaledBoxExtent() * 2.0f;
	BaseMeshScale = FVector(
		BarrierSize.X / FMath::Max(MeshSize.X, 1.0f),
		BarrierSize.Y / FMath::Max(MeshSize.Y, 1.0f),
		BarrierSize.Z / FMath::Max(MeshSize.Z, 1.0f));
	BarrierHalfHeight = BarrierBox->GetScaledBoxExtent().Z;
	SetWorldScale3D(BaseMeshScale);

	SetMaterial(0, Material);
	DynamicMaterial = CreateAndSetMaterialInstanceDynamic(0);
	if (!DynamicMaterial)
	{
		UE_LOG(
			LogLastFPSRoomBarrierPresentation,
			Error,
			TEXT("배리어 '%s'의 동적 머티리얼 생성에 실패했습니다."),
			*BarrierVolume.GetPathName());
		return;
	}

	DissolveParameter = Settings.DissolveParameter;
	DissolveDuration = FMath::Max(Settings.DissolveDuration, 0.0f);
	DynamicMaterial->SetVectorParameterValue(Settings.ColorParameter, Settings.Color);
	DynamicMaterial->SetScalarParameterValue(Settings.OpacityParameter, Settings.Opacity);
	DynamicMaterial->SetScalarParameterValue(
		Settings.FresnelPowerParameter,
		FMath::Max(Settings.FresnelPower, 0.1f));
	DynamicMaterial->SetScalarParameterValue(
		Settings.FresnelIntensityParameter,
		FMath::Max(Settings.FresnelIntensity, 0.0f));

	bConfigured = true;
	bVisualActive = false;
	bDissolving = false;
	DissolveElapsed = 0.0f;
	ApplyDissolveProgress(1.0f);
	SetVisibility(false, true);
	SetComponentTickEnabled(false);
}

void ULastFPSRoomBarrierPresentationComponent::SetBarrierActive(const bool bActive)
{
	if (!bConfigured)
	{
		return;
	}

	if (bActive)
	{
		bVisualActive = true;
		bDissolving = false;
		DissolveElapsed = 0.0f;
		ApplyDissolveProgress(0.0f);
		SetVisibility(true, true);
		SetComponentTickEnabled(false);
		return;
	}

	if (!bVisualActive)
	{
		ApplyDissolveProgress(1.0f);
		SetVisibility(false, true);
		return;
	}

	if (DissolveDuration <= KINDA_SMALL_NUMBER)
	{
		FinishDissolve();
		return;
	}

	bDissolving = true;
	DissolveElapsed = 0.0f;
	SetComponentTickEnabled(true);
}

void ULastFPSRoomBarrierPresentationComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bDissolving)
	{
		SetComponentTickEnabled(false);
		return;
	}

	DissolveElapsed += FMath::Max(DeltaTime, 0.0f);
	const float Progress = FMath::Clamp(DissolveElapsed / DissolveDuration, 0.0f, 1.0f);
	ApplyDissolveProgress(Progress);

	if (Progress >= 1.0f)
	{
		FinishDissolve();
	}
}

void ULastFPSRoomBarrierPresentationComponent::ApplyDissolveProgress(const float Progress)
{
	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	// 아래쪽 경계는 고정하고 위쪽 경계만 내려야 위에서 아래로 사라져 보인다.
	FVector DissolveScale = BaseMeshScale;
	DissolveScale.Z *= 1.0f - ClampedProgress;
	SetWorldScale3D(DissolveScale);
	SetWorldLocation(
		BaseWorldLocation - BarrierUpDirection * BarrierHalfHeight * ClampedProgress);

	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(
			DissolveParameter,
			ClampedProgress);
	}
}

void ULastFPSRoomBarrierPresentationComponent::FinishDissolve()
{
	bDissolving = false;
	bVisualActive = false;
	DissolveElapsed = DissolveDuration;
	ApplyDissolveProgress(1.0f);
	SetVisibility(false, true);
	SetComponentTickEnabled(false);
}
