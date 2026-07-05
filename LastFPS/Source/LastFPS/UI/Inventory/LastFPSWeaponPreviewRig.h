#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSWeaponPreviewRig.generated.h"

class USceneCaptureComponent2D;
class USkeletalMeshComponent;
class USkeletalMesh;
class UPointLightComponent;
class UTextureRenderTarget2D;

/**
 * 무기 3D 프리뷰용 로컬 리그 액터. 허브 월드의 오프스크린 위치에 **비복제**로 스폰되어,
 * SceneCapture2D 로 무기 메시만(ShowOnlyList) RenderTarget 에 촬영한다.
 * 프리뷰 위젯이 그 RenderTarget 을 UImage 로 표시하고, 드래그로 AddYaw 회전시킨다. 위젯이 닫히면 파괴된다.
 */
UCLASS()
class LASTFPS_API ALastFPSWeaponPreviewRig : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSWeaponPreviewRig();

	/** 무기 메시 설정 + RenderTarget 준비 + show-only 구성. 생성된 RenderTarget 을 반환. */
	UTextureRenderTarget2D* InitPreview(USkeletalMesh* WeaponMesh);

	/** 피벗(무기)을 좌우로 회전. */
	void AddYaw(float DeltaDegrees);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> RootScene;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Pivot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> WeaponMeshComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> KeyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> FillLight;

	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	/** RenderTarget 해상도(정사각). */
	UPROPERTY(EditDefaultsOnly, Category="Preview") int32 RenderTargetSize = 768;
};
