#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LastFPSGameDataSet.generated.h"

class UDataTable;
class ULastFPSCharacterRoster;
class ULastFPSScreenRegistry;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSDataTableReference
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data",
		meta=(Categories="Data.Table"))
	FGameplayTag TableId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data")
	TSoftObjectPtr<UDataTable> Table;
};

/**
 * 도메인 서브시스템이 사용할 데이터 테이블과 전역 카탈로그의 로딩 경계를 정의한다.
 * 테이블의 해석과 런타임 상태는 각 도메인 서브시스템이 소유한다.
 */
UCLASS(BlueprintType, Const)
class LASTFPS_API ULastFPSGameDataSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType PrimaryAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	const FLastFPSDataTableReference* FindTable(FGameplayTag TableId) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Global",
		meta=(AssetBundles="Startup"))
	TSoftObjectPtr<ULastFPSCharacterRoster> CharacterRoster;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Global",
		meta=(AssetBundles="Startup"))
	TSoftObjectPtr<ULastFPSScreenRegistry> ScreenRegistry;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tables",
		meta=(TitleProperty="TableId", AssetBundles="Startup"))
	TArray<FLastFPSDataTableReference> StartupTables;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tables",
		meta=(TitleProperty="TableId", AssetBundles="Game"))
	TArray<FLastFPSDataTableReference> GameTables;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tables",
		meta=(TitleProperty="TableId", AssetBundles="UI"))
	TArray<FLastFPSDataTableReference> UITables;
};
