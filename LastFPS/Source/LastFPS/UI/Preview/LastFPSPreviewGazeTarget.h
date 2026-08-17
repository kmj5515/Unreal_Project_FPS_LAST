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

	/**
	 * 이 자리에 무기가 붙어 있는지 알린다. 무장/비무장 대기 자세를 나누는 데 쓴다.
	 *
	 * 무기 메시는 캐릭터가 아니라 무대 액터가 소유하고 소켓으로만 매달리므로, 애님 인스턴스가
	 * 자기 컴포넌트를 뒤져서는 알 수 없다. 무대가 아는 사실이니 무대가 넘긴다.
	 *
	 * 무대는 어떤 무기인지 알리지 않는다. 무기 종류마다 자세를 나누려면 그 지식은 무기 정의가
	 * 들고 있어야 하고, 여기에 무기 목록이 들어오면 무기가 늘 때마다 이 계약이 흔들린다.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Preview")
	void SetPreviewEquippedWeapon(bool bHasEquippedWeapon);
};
