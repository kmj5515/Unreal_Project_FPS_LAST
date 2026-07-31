#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/PrimaryAssetId.h"
#include "LastFPSGameDataSubsystem.generated.h"

class UDataTable;
class ULastFPSCharacterRoster;
class ULastFPSGameDataSet;
class ULastFPSScreenRegistry;
struct FLastFPSDataTableReference;

/**
 * GameDataSet Primary Asset을 로드하고 안정적인 Gameplay Tag로 공유 데이터를 제공한다.
 * 개별 기능은 에셋 경로나 구체 DataSet을 알지 않고 이 계약만 소비한다.
 */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSGameDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UDataTable* FindTable(FGameplayTag TableId);
	const ULastFPSCharacterRoster* GetCharacterRoster();
	ULastFPSScreenRegistry* GetScreenRegistry();

private:
	bool LoadDataSet(const FPrimaryAssetId& DataSetId);
	const FLastFPSDataTableReference* FindTableReference(FGameplayTag TableId) const;
	void EnsureStartupDataSetLoaded();

	/** 다른 GameInstanceSubsystem이 초기화될 때 즉시 사용할 전역 데이터 묶음이다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Game Data", meta=(AllowedTypes="GameDataSet"))
	FPrimaryAssetId StartupDataSetId;

	/** 요청한 태그가 아직 없을 때 순서대로 확인하며 필요한 묶음만 지연 로드한다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Game Data", meta=(AllowedTypes="GameDataSet"))
	TArray<FPrimaryAssetId> OnDemandDataSetIds;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSGameDataSet>> LoadedDataSets;

	TSet<FPrimaryAssetId> AttemptedDataSetIds;
};
