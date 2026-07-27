#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSDestinationFeature.h"
#include "GameplayTagContainer.h"
#include "LastFPSActorPoolProfile.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSActorPoolEntry
{
	GENERATED_BODY()

	/** Blueprint 요청과 로그에서 사용하는 안정적인 풀 식별자다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pool", meta=(Categories="Actor.Pool"))
	FGameplayTag PoolId;

	/** 이 풀에서 생성하고 재사용할 Actor 클래스다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pool")
	TSoftClassPtr<AActor> ActorClass;

	/** 목적지 진입 시 미리 생성할 개수다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pool",
		meta=(ClampMin="0", UIMin="0"))
	int32 InitialSize = 0;

	/** 동시에 유지할 수 있는 총 인스턴스 수다. 0이면 InitialSize를 상한으로 사용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pool",
		meta=(ClampMin="0", UIMin="0"))
	int32 MaxSize = 0;

	/** 초기 인스턴스의 렌더 컴포넌트를 로딩 화면 뒤에서 준비한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pool")
	bool bRenderWarmup = true;
};

/**
 * Mode별 Actor Pool 구성이다.
 * DestinationContentSet의 Features에 넣으면 클래스 로드와 풀 생성이 같은 데이터에서 파생된다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSActorPoolProfile : public ULastFPSDestinationFeature
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pool")
	TArray<FLastFPSActorPoolEntry> Pools;

	virtual void CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const override;

	bool IsConfigurationValid(FString& OutFailureReason) const;
};
