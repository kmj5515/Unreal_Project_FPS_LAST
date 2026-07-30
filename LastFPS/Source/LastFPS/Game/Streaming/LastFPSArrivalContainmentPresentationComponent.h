#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LastFPSArrivalContainmentPresentationComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
struct FLastFPSArrivalContainmentPresentationSettings;

/** 도착 대기 상태를 반투명 원통과 교차 경고 띠로 표현한다. */
UCLASS(ClassGroup=(LastFPS))
class LASTFPS_API ULastFPSArrivalContainmentPresentationComponent
	: public USceneComponent
{
	GENERATED_BODY()

public:
	ULastFPSArrivalContainmentPresentationComponent();

	void Configure(
		const FLastFPSArrivalContainmentPresentationSettings& InSettings,
		UStaticMesh& Mesh,
		UMaterialInterface& Material);

	void ShowAt(const FTransform& PlayerTransform);
	void Hide();

private:
	UStaticMeshComponent* CreateMeshPart(
		FName ComponentName,
		UStaticMesh& Mesh,
		UMaterialInterface& Material,
		const FLinearColor& Color,
		float Opacity,
		FName ColorParameter,
		FName OpacityParameter);
	void ResetPresentation();
	void ApplyRequestedVisibility();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> MeshParts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	FVector PlayerOriginOffset = FVector(0.f, 0.f, -90.f);
	FTransform RequestedPlayerTransform = FTransform::Identity;
	bool bConfigured = false;
	bool bVisibilityRequested = false;
};
