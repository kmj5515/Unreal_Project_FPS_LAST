#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "LastFPSBattleDefinition.generated.h"

class ULastFPSDestinationContentSet;
class ULastFPSDropProfile;
class UTexture2D;

/**
 * 전투 진입에 필요한 불변 설정.
 * 세션 검색 결과나 참가 상태 같은 런타임 값은 이 에셋에 저장하지 않는다.
 */
UCLASS(BlueprintType, Const)
class LASTFPS_API ULastFPSBattleDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType PrimaryAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** CommonSession이 호스트할 최상위 월드 에셋 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Battle", meta=(AllowedTypes="Map"))
	FPrimaryAssetId MapId;

	/** 서로 호환되는 퀵플레이 세션을 구분하는 안정적인 식별자 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Matchmaking",
		meta=(Categories="Matchmaking.Pool"))
	FGameplayTag MatchmakingPoolTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Matchmaking", meta=(ClampMin="1"))
	int32 MaxPlayerCount = 3;

	/** 전투 월드가 준비해야 할 기능과 에셋 묶음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Battle",
		meta=(AssetBundles="Game"))
	TSoftObjectPtr<ULastFPSDestinationContentSet> ContentSet;

	/** 이 전투 레벨의 적 처치 드랍 규칙. 비우면 드랍 없음. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Battle",
		meta=(AssetBundles="Game"))
	TSoftObjectPtr<ULastFPSDropProfile> DropProfile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
	FText DisplayName;

	/** String Table에서 전투 레벨 표시 이름을 조회할 키. 지정하면 DisplayName보다 우선한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
	FName DisplayNameStringTableKey;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation",
		meta=(AssetBundles="UI"))
	TSoftObjectPtr<UTexture2D> Thumbnail;
};
