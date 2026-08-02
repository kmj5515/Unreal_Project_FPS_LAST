#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LastFPSPreviewGazeTarget.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class ULastFPSPreviewGazeTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 * 프리뷰 무대가 "지금 어디를 보면 되는지"를 애님 인스턴스에 알려 주는 계약.
 *
 * 무대는 Look At 노드도 본 이름도 알지 않는다. 월드 좌표 하나만 넘기고, 그것을 어떻게 쓸지는
 * 애님 블루프린트가 정한다. 그래야 캐릭터마다 목·눈 본 구성이 달라도 무대 코드가 그대로다.
 *
 * 타깃이 월드 좌표라 드래그로 몸을 돌려도 시선은 카메라에 남는다. 매 프레임 갱신할 필요는 없고,
 * 시점이 바뀌거나 대상이 바뀔 때만 다시 넘어온다.
 */
class LASTFPS_API ILastFPSPreviewGazeTarget
{
	GENERATED_BODY()

public:
	/**
	 * @param GazeWorldLocation 바라볼 지점(월드). 보통 활성 시점 카메라의 위치다.
	 * @param GazeAlpha         시선을 얼마나 적용할지(0~1). Look At 노드의 Alpha 에 그대로 연결한다.
	 *                          0 이면 원래 포즈, 1 이면 완전히 그 지점을 본다. 시점마다 다르게 줄 수 있다.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Preview")
	void SetPreviewGazeTarget(const FVector& GazeWorldLocation, float GazeAlpha);
};
