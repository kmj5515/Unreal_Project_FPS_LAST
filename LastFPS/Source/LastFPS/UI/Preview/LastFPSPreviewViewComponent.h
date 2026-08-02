#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameplayTagContainer.h"
#include "LastFPSPreviewViewComponent.generated.h"

/**
 * 프리뷰 무대의 시점 하나. 무대 BP 에 필요한 만큼 붙이고 각자 태그를 달아 둔다.
 *
 * 대상마다 필요한 거리와 화각이 달라(무기는 가까이, 캐릭터는 전신) 값 하나로는 맞출 수 없다.
 * 시점을 카메라 컴포넌트로 배치해 두면 위치·화각·포스트프로세스를 에디터에서 눈으로 맞출 수 있고,
 * 시점이 늘어나도 코드는 그대로다. 무대는 태그로 지목받아 그 카메라만 켠다.
 */
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent, DisplayName="LastFPS Preview View"))
class LASTFPS_API ULastFPSPreviewViewComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	ULastFPSPreviewViewComponent();

	FGameplayTag GetViewTag() const { return ViewTag; }
	FGameplayTag GetFocusSlotTag() const { return FocusSlotTag; }
	bool ShouldLookAtFocusSlot() const { return bLookAtFocusSlot; }
	bool ShouldFocusDepthOfField() const { return bFocusDepthOfField; }
	float GetGazeAlpha() const { return GazeAlpha; }

protected:
	/** 이 시점을 지목할 때 쓰는 태그. 같은 태그가 둘이면 먼저 찾은 것이 쓰인다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview", meta=(Categories="UI.Preview.View"))
	FGameplayTag ViewTag;

	/**
	 * 이 시점이 주목할 자리. 비워 두면 아래 두 옵션은 아무 일도 하지 않는다.
	 *
	 * 자리 원점이 아니라 그 자리에 올라온 메시의 바운드 중심을 겨눈다.
	 * 캐릭터는 발이 원점이라 원점을 겨누면 바닥을 보게 된다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview|Focus", meta=(Categories="UI.Preview.Slot"))
	FGameplayTag FocusSlotTag;

	/** 켜면 대상을 향하도록 회전을 자동으로 맞춘다. 끄면 BP 에 배치한 각도를 그대로 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview|Focus")
	bool bLookAtFocusSlot = false;

	/**
	 * 켜면 대상까지의 거리를 피사계 심도 초점 거리로 쓴다. 배경이 흐려져 대상이 분리된다.
	 * 조리개(F-Stop)는 이 컴포넌트의 PostProcessSettings 에서 직접 잡는다. 여기서는 거리만 맞춘다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview|Focus")
	bool bFocusDepthOfField = false;

	/**
	 * 이 시점에서 대상이 카메라를 얼마나 쳐다볼지(0~1). ABP 의 Look At 노드 Alpha 로 그대로 전달된다.
	 *
	 * 0 이면 원래 포즈를 유지하고 1 이면 완전히 카메라를 본다. 시점마다 다르게 줄 수 있어,
	 * 정면 시점은 1 로 두고 옆에서 잡는 연출 시점은 낮춰 목이 꺾이지 않게 할 수 있다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview|Focus", meta=(ClampMin="0.0", ClampMax="1.0"))
	float GazeAlpha = 1.f;
};
