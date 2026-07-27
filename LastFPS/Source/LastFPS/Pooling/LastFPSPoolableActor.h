#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LastFPSPoolableActor.generated.h"

/**
 * Actor Pool이 구체 Actor 종류를 알지 않고 수명 주기를 전달하는 공통 계약이다.
 * C++와 Blueprint 구현 모두 동일한 진입점을 사용한다.
 */
UINTERFACE(BlueprintType)
class LASTFPS_API ULastFPSPoolableActor : public UInterface
{
	GENERATED_BODY()
};

class LASTFPS_API ILastFPSPoolableActor
{
	GENERATED_BODY()

public:
	/** 풀에서 활성화된 직후 호출된다. 호출자는 이후 기능별 초기화 데이터를 주입한다. */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Pool")
	void OnAcquiredFromPool();

	/** 비활성 풀로 반환되기 직전에 호출된다. 타이머·델리게이트·기능 상태를 정리한다. */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Pool")
	void OnReleasedToPool();

	/**
	 * 로딩 화면 뒤에서 렌더 컴포넌트만 준비할 때 호출된다.
	 * 충돌이나 게임플레이를 켜지 않고 필요한 시각 컴포넌트만 준비해야 한다.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Pool")
	void OnPrepareForPoolRenderWarmup();
};
