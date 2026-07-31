#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LastFPSRenderWarmupSource.generated.h"

class AActor;
class UWorld;

/**
 * 콘텐츠 데이터가 자신에게 필요한 렌더 워밍업 인스턴스를 제공하는 공통 계약이다.
 * 중앙 로더는 구체적인 무기, 스킬 또는 발사체 타입을 알지 않고 이 계약만 소비한다.
 */
UINTERFACE(MinimalAPI)
class ULastFPSRenderWarmupSource : public UInterface
{
	GENERATED_BODY()
};

class LASTFPS_API ILastFPSRenderWarmupSource
{
	GENERATED_BODY()

public:
	/**
	 * 이미 로드된 설정만 사용해 일시적인 렌더 워밍업 Actor를 만든다.
	 * 생성된 Actor의 수명은 호출한 중앙 로더가 소유한다.
	 */
	virtual void CreateRenderWarmupActors(
		UWorld& World,
		const FTransform& SpawnTransform,
		TArray<AActor*>& OutActors) const = 0;
};
