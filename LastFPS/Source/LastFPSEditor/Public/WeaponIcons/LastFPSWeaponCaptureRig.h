#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSWeaponCaptureRig.generated.h"

class UPointLightComponent;
class USceneComponent;
class USkeletalMeshComponent;

/**
 * 아이콘 촬영용 조명 하나의 배치와 세기다.
 *
 * 조명을 절대 좌표로 두면 카메라 거리는 피사체 크기에 맞춰 조정되는데 조명만 제자리에 남아
 * 무기 크기마다 다른 조명이 된다. 방향과 바운드 배수로 표현해야 크기가 달라도 같은 각도가 나온다.
 * 방향은 카메라 기준(X=전방, Y=우측, Z=상단)이라 촬영 각도를 바꿔도 조명이 함께 따라간다.
 */
USTRUCT()
struct FLastFPSIconLightSetup
{
	GENERATED_BODY()

	/** 카메라 기준 방향. 정규화해서 쓰므로 길이는 무시된다. */
	UPROPERTY(EditAnywhere, Category="Light")
	FVector Direction = FVector(-1.f, -1.f, 1.f);

	/** 피사체 바운드 반경의 몇 배 거리에 둘지 결정한다. */
	UPROPERTY(EditAnywhere, Category="Light", meta=(ClampMin="0.5"))
	float DistanceScale = 3.f;

	UPROPERTY(EditAnywhere, Category="Light")
	FLinearColor Color = FLinearColor::White;

	/** 기준 거리에서의 광량. 실제 거리에 맞춰 보정되므로 무기 크기가 달라도 밝기는 유지된다. */
	UPROPERTY(EditAnywhere, Category="Light", meta=(ClampMin="0.0"))
	float Intensity = 5000.f;

	UPROPERTY(EditAnywhere, Category="Light")
	bool bCastShadows = false;
};

/** 에디터 전용 소유자 필터의 영향을 받지 않고 무기 메시와 조명을 렌더링하는 일시적 촬영 리그다. */
UCLASS(Transient, NotBlueprintable)
class LASTFPSEDITOR_API ALastFPSWeaponCaptureRig final : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSWeaponCaptureRig();
	virtual bool IsEditorOnly() const override { return false; }

	/**
	 * 피사체 크기와 촬영 각도에 맞춰 3점 조명을 다시 배치한다.
	 *
	 * 메시 바운드가 확정된 뒤에 호출해야 한다. 생성 시점에는 무엇을 찍을지 아직 모른다.
	 *
	 * @param CameraRotation 조명 방향의 기준이 되는 카메라 회전.
	 * @param BoundsRadius   피사체 바운드 반경. 조명 거리와 광량이 이 값을 따라간다.
	 */
	void ConfigureLighting(
		const FLastFPSIconLightSetup& KeySetup,
		const FLastFPSIconLightSetup& FillSetup,
		const FLastFPSIconLightSetup& RimSetup,
		const FQuat& CameraRotation,
		float BoundsRadius);

	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

private:
	static void ApplyLightSetup(
		UPointLightComponent* Light,
		const FLastFPSIconLightSetup& Setup,
		const FQuat& CameraRotation,
		float BoundsRadius);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RigRoot;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> KeyLight;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> FillLight;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> RimLight;
};
