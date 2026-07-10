#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSWeaponPreviewRig.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class USkeletalMesh;

/**
 * 무기 3D 프리뷰용 로컬 리그 액터.
 * SceneCapture/RenderTarget 방식을 걷어내고, 프리뷰 카메라(UCameraComponent)를 가진 리그로 동작한다.
 * 프리뷰를 열 때 플레이어 뷰타깃을 이 리그로 전환(SetViewTargetWithBlend)해 화면 전체로 무기를 본다.
 * 무기(피벗의 자식)를 드래그로 AddYaw 회전시킨다. 리그는 인벤토리가 소유·재사용한다.
 */
UCLASS()
class LASTFPS_API ALastFPSWeaponPreviewRig : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSWeaponPreviewRig();

	/** 프리뷰할 무기 메시 설정 + 피벗 회전 초기화. */
	void InitPreview(USkeletalMesh* WeaponMesh);

	/** 피벗(무기)을 좌우로 회전. */
	void AddYaw(float DeltaDegrees);

	/** 뷰타깃 전환에 쓸 프리뷰 카메라. */
	UCameraComponent* GetPreviewCamera() const
	{
		return PreviewCamera;
	}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) 
	TObjectPtr<USceneComponent> RootScene;
	UPROPERTY(VisibleAnywhere) 
	TObjectPtr<USceneComponent> Pivot;
	UPROPERTY(VisibleAnywhere) 
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComp;
	UPROPERTY(VisibleAnywhere) 
	TObjectPtr<UCameraComponent> PreviewCamera;
};
