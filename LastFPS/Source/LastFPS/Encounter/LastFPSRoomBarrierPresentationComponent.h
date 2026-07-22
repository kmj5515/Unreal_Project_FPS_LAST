#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "LastFPSRoomBarrierPresentationComponent.generated.h"

class ATriggerBox;
class UMaterialInstanceDynamic;
struct FLastFPSRoomBarrierPresentationSettings;

/**
 * 룸 출구 충돌 박스와 동일한 위치에 홀로그램을 표시하고,
 * 출구가 열리면 위에서 아래로 소거하는 시각 표현만 담당한다.
 */
UCLASS(ClassGroup=(Encounter))
class LASTFPS_API ULastFPSRoomBarrierPresentationComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	ULastFPSRoomBarrierPresentationComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	void Configure(
		ATriggerBox& BarrierVolume,
		const FLastFPSRoomBarrierPresentationSettings& Settings);
	void SetBarrierActive(bool bActive);

private:
	void ApplyDissolveProgress(float Progress);
	void FinishDissolve();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	FName DissolveParameter = TEXT("DissolveProgress");
	FVector BaseMeshScale = FVector::OneVector;
	FVector BaseWorldLocation = FVector::ZeroVector;
	FVector BarrierUpDirection = FVector::UpVector;
	float BarrierHalfHeight = 0.0f;
	float DissolveDuration = 0.7f;
	float DissolveElapsed = 0.0f;
	bool bConfigured = false;
	bool bVisualActive = false;
	bool bDissolving = false;
};
