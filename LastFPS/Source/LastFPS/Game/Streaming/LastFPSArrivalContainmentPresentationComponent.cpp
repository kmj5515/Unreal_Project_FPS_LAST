#include "Game/Streaming/LastFPSArrivalContainmentPresentationComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Game/Streaming/LastFPSStreamingLevelTransitionSettings.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ULastFPSArrivalContainmentPresentationComponent::
	ULastFPSArrivalContainmentPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULastFPSArrivalContainmentPresentationComponent::Configure(
	const FLastFPSArrivalContainmentPresentationSettings& InSettings,
	UStaticMesh& Mesh,
	UMaterialInterface& Material)
{
	ResetPresentation();
	PlayerOriginOffset = InSettings.Offset;

	const FVector MeshSize =
		Mesh.GetBounds().BoxExtent * 2.f;
	const FVector FieldScale(
		(InSettings.Radius * 2.f)
			/ FMath::Max(MeshSize.X, 1.f),
		(InSettings.Radius * 2.f)
			/ FMath::Max(MeshSize.Y, 1.f),
		InSettings.Height
			/ FMath::Max(MeshSize.Z, 1.f));

	UStaticMeshComponent* Field = CreateMeshPart(
		TEXT("ArrivalContainmentField"),
		Mesh,
		Material,
		InSettings.FieldColor,
		InSettings.FieldOpacity,
		InSettings.ColorParameter,
		InSettings.OpacityParameter);
	if (!Field)
	{
		return;
	}

	Field->SetRelativeLocation(
		FVector(0.f, 0.f, InSettings.Height * 0.5f));
	Field->SetRelativeScale3D(FieldScale);

	const int32 BandCount =
		FMath::Clamp(InSettings.BandCount, 1, 16);
	const float BandHeight =
		FMath::Clamp(
			InSettings.BandHeight,
			1.f,
			InSettings.Height);
	const float AvailableTravel =
		FMath::Max(InSettings.Height - BandHeight, 0.f);

	for (int32 BandIndex = 0;
		BandIndex < BandCount;
		++BandIndex)
	{
		const float Alpha = BandCount > 1
			? static_cast<float>(BandIndex)
				/ static_cast<float>(BandCount - 1)
			: 0.5f;
		const float BandCenterZ =
			BandHeight * 0.5f + AvailableTravel * Alpha;
		const FLinearColor& BandColor =
			BandIndex % 2 == 0
				? InSettings.PrimaryBandColor
				: InSettings.SecondaryBandColor;
		const FName ComponentName =
			*FString::Printf(
				TEXT("ArrivalContainmentBand_%02d"),
				BandIndex);

		UStaticMeshComponent* Band = CreateMeshPart(
			ComponentName,
			Mesh,
			Material,
			BandColor,
			InSettings.BandOpacity,
			InSettings.ColorParameter,
			InSettings.OpacityParameter);
		if (!Band)
		{
			continue;
		}

		FVector BandScale = FieldScale;
		BandScale.X *= 0.985f;
		BandScale.Y *= 0.985f;
		BandScale.Z =
			BandHeight / FMath::Max(MeshSize.Z, 1.f);
		Band->SetRelativeLocation(
			FVector(0.f, 0.f, BandCenterZ));
		Band->SetRelativeScale3D(BandScale);
	}

	for (UMaterialInstanceDynamic* DynamicMaterial
		: DynamicMaterials)
	{
		if (!DynamicMaterial)
		{
			continue;
		}

		DynamicMaterial->SetScalarParameterValue(
			InSettings.DissolveParameter,
			0.f);
		DynamicMaterial->SetScalarParameterValue(
			InSettings.FresnelPowerParameter,
			FMath::Max(InSettings.FresnelPower, 0.1f));
		DynamicMaterial->SetScalarParameterValue(
			InSettings.FresnelIntensityParameter,
			FMath::Max(InSettings.FresnelIntensity, 0.f));
	}

	bConfigured = true;
	ApplyRequestedVisibility();
}

void ULastFPSArrivalContainmentPresentationComponent::ShowAt(
	const FTransform& PlayerTransform)
{
	RequestedPlayerTransform = PlayerTransform;
	bVisibilityRequested = true;
	ApplyRequestedVisibility();
}

void ULastFPSArrivalContainmentPresentationComponent::Hide()
{
	bVisibilityRequested = false;
	ApplyRequestedVisibility();
}

UStaticMeshComponent*
ULastFPSArrivalContainmentPresentationComponent::CreateMeshPart(
	const FName ComponentName,
	UStaticMesh& Mesh,
	UMaterialInterface& Material,
	const FLinearColor& Color,
	const float Opacity,
	const FName ColorParameter,
	const FName OpacityParameter)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	UStaticMeshComponent* MeshPart =
		NewObject<UStaticMeshComponent>(
			Owner,
			MakeUniqueObjectName(
				Owner,
				UStaticMeshComponent::StaticClass(),
				ComponentName));
	if (!MeshPart)
	{
		return nullptr;
	}

	MeshPart->SetupAttachment(this);
	MeshPart->SetStaticMesh(&Mesh);
	MeshPart->SetMaterial(0, &Material);
	MeshPart->SetCollisionProfileName(
		UCollisionProfile::NoCollision_ProfileName);
	MeshPart->SetGenerateOverlapEvents(false);
	MeshPart->SetCanEverAffectNavigation(false);
	MeshPart->SetCastShadow(false);
	MeshPart->SetReceivesDecals(false);
	MeshPart->SetTranslucentSortPriority(110);
	MeshPart->SetVisibility(false, true);
	MeshPart->RegisterComponent();

	UMaterialInstanceDynamic* DynamicMaterial =
		MeshPart->CreateAndSetMaterialInstanceDynamic(0);
	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(
			ColorParameter,
			Color);
		DynamicMaterial->SetScalarParameterValue(
			OpacityParameter,
			FMath::Clamp(Opacity, 0.f, 1.f));
		DynamicMaterials.Add(DynamicMaterial);
	}

	MeshParts.Add(MeshPart);
	return MeshPart;
}

void ULastFPSArrivalContainmentPresentationComponent::
	ResetPresentation()
{
	for (UStaticMeshComponent* MeshPart : MeshParts)
	{
		if (IsValid(MeshPart))
		{
			MeshPart->DestroyComponent();
		}
	}

	MeshParts.Reset();
	DynamicMaterials.Reset();
	bConfigured = false;
}

void ULastFPSArrivalContainmentPresentationComponent::
	ApplyRequestedVisibility()
{
	if (!bConfigured)
	{
		return;
	}

	if (bVisibilityRequested)
	{
		const FVector WorldOffset =
			RequestedPlayerTransform
				.TransformVectorNoScale(PlayerOriginOffset);
		SetWorldLocationAndRotation(
			RequestedPlayerTransform.GetLocation()
				+ WorldOffset,
			RequestedPlayerTransform.GetRotation());
	}

	for (UStaticMeshComponent* MeshPart : MeshParts)
	{
		if (IsValid(MeshPart))
		{
			MeshPart->SetVisibility(
				bVisibilityRequested,
				true);
		}
	}
}
