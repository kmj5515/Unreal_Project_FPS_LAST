#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSDestinationContentSet.generated.h"

/**
 * 맵 진입 전에 준비되어야 하는 콘텐츠 목록.
 * "이 맵이 무엇을 요구하는가"는 맵마다 다른 규칙이므로 GameMode 가 이 에셋을 가리킨다.
 * (InitialScreenTag / LevelRestrictionEffect 와 같은 소유 규칙)
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSDestinationContentSet : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 데이터 테이블·텍스처·데이터 에셋 등. 로드가 끝날 때까지 로딩 화면을 유지한다. */
    UPROPERTY(EditDefaultsOnly, Category="Content")
    TArray<TSoftObjectPtr<UObject>> RequiredAssets;

    /**
     * 위젯·Pawn 같은 Blueprint 클래스는 반드시 이쪽에 넣는다.
     * RequiredAssets 에 넣으면 생성 클래스(_C)가 아니라 UBlueprint 를 가리켜
     * 쿠킹된 빌드에서 해석되지 않는다.
     */
    UPROPERTY(EditDefaultsOnly, Category="Content")
    TArray<TSoftClassPtr<UObject>> RequiredClasses;

    /** 두 목록의 유효한 항목만 모은다. 비어 있으면 게이트가 걸리지 않는다. */
    void CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const;
};
