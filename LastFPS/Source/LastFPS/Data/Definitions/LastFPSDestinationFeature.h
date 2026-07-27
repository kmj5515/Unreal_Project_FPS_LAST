#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSDestinationFeature.generated.h"

/**
 * 목적지에 종속된 기능 설정이 중앙 로딩에 필요한 경로를 제공하는 공통 계약이다.
 * DestinationContentSet은 구체 기능을 열거하지 않고 이 계약만 소비한다.
 */
UCLASS(Abstract, BlueprintType)
class LASTFPS_API ULastFPSDestinationFeature : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual void CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const
	{
	}
};
