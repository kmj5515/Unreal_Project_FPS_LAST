#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"

#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"
#include "Data/Definitions/LastFPSCharacterRoster.h"
#include "Data/Definitions/LastFPSGameDataSet.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/StreamableManager.h"
#include "UI/Framework/LastFPSScreenRegistry.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSGameDataSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSGameData, Log, All);

void ULastFPSGameDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureStartupDataSetLoaded();
}

void ULastFPSGameDataSubsystem::Deinitialize()
{
	TArray<FPrimaryAssetId> DataSetIdsToUnload;
	DataSetIdsToUnload.Reserve(LoadedDataSets.Num());
	for (const ULastFPSGameDataSet* DataSet : LoadedDataSets)
	{
		if (DataSet)
		{
			DataSetIdsToUnload.AddUnique(DataSet->GetPrimaryAssetId());
		}
	}
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		AssetManager->UnloadPrimaryAssets(DataSetIdsToUnload);
	}

	AttemptedDataSetIds.Reset();
	LoadedDataSets.Reset();
	Super::Deinitialize();
}

UDataTable* ULastFPSGameDataSubsystem::FindTable(const FGameplayTag TableId)
{
	if (!TableId.IsValid())
	{
		return nullptr;
	}

	EnsureStartupDataSetLoaded();
	if (const FLastFPSDataTableReference* Reference = FindTableReference(TableId))
	{
		if (UDataTable* Table = Reference->Table.Get())
		{
			return Table;
		}

		UE_LOG(LogLastFPSGameData, Error, TEXT("GameDataSet 번들에 등록된 테이블이 메모리에 없습니다: TableId=%s, Path=%s"),
			*TableId.ToString(), *Reference->Table.ToString());
		return nullptr;
	}

	for (const FPrimaryAssetId& DataSetId : OnDemandDataSetIds)
	{
		if (!AttemptedDataSetIds.Contains(DataSetId))
		{
			LoadDataSet(DataSetId);
		}

		if (const FLastFPSDataTableReference* Reference = FindTableReference(TableId))
		{
			if (UDataTable* Table = Reference->Table.Get())
			{
				return Table;
			}

			UE_LOG(LogLastFPSGameData, Error, TEXT("지연 로드한 GameDataSet의 테이블이 메모리에 없습니다: TableId=%s, Path=%s"),
				*TableId.ToString(), *Reference->Table.ToString());
			return nullptr;
		}
	}

	UE_LOG(LogLastFPSGameData, Error, TEXT("등록된 GameDataSet에서 테이블을 찾지 못했습니다: TableId=%s"), *TableId.ToString());
	return nullptr;
}

const ULastFPSCharacterRoster* ULastFPSGameDataSubsystem::GetCharacterRoster()
{
	EnsureStartupDataSetLoaded();
	for (const ULastFPSGameDataSet* DataSet : LoadedDataSets)
	{
		if (DataSet && DataSet->CharacterRoster.Get())
		{
			return DataSet->CharacterRoster.Get();
		}
	}

	UE_LOG(LogLastFPSGameData, Error, TEXT("로드된 GameDataSet에 CharacterRoster가 없습니다."));
	return nullptr;
}

ULastFPSScreenRegistry* ULastFPSGameDataSubsystem::GetScreenRegistry()
{
	EnsureStartupDataSetLoaded();
	for (const ULastFPSGameDataSet* DataSet : LoadedDataSets)
	{
		if (DataSet && DataSet->ScreenRegistry.Get())
		{
			return DataSet->ScreenRegistry.Get();
		}
	}

	UE_LOG(LogLastFPSGameData, Error, TEXT("로드된 GameDataSet에 ScreenRegistry가 없습니다."));
	return nullptr;
}

bool ULastFPSGameDataSubsystem::LoadDataSet(const FPrimaryAssetId& DataSetId)
{
	if (!DataSetId.IsValid() || DataSetId.PrimaryAssetType != LastFPSPrimaryAssetTypes::GameDataSet)
	{
		UE_LOG(LogLastFPSGameData, Error, TEXT("GameDataSet Primary Asset ID가 올바르지 않습니다: %s"), *DataSetId.ToString());
		return false;
	}

	for (const ULastFPSGameDataSet* LoadedDataSet : LoadedDataSets)
	{
		if (LoadedDataSet && LoadedDataSet->GetPrimaryAssetId() == DataSetId)
		{
			return true;
		}
	}

	AttemptedDataSetIds.Add(DataSetId);
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FName> BundlesToLoad;
	BundlesToLoad.Add(LastFPSAssetBundles::Startup);
	BundlesToLoad.Add(LastFPSAssetBundles::Game);
	BundlesToLoad.Add(LastFPSAssetBundles::UI);

	TSharedPtr<FStreamableHandle> Handle = AssetManager.LoadPrimaryAsset(DataSetId, BundlesToLoad);
	if (Handle.IsValid())
	{
		Handle->WaitUntilComplete();
	}

	ULastFPSGameDataSet* LoadedDataSet = Cast<ULastFPSGameDataSet>(AssetManager.GetPrimaryAssetObject(DataSetId));
	if (!LoadedDataSet)
	{
		UE_LOG(LogLastFPSGameData, Error, TEXT("GameDataSet 로드가 완료됐지만 객체를 찾지 못했습니다: %s"), *DataSetId.ToString());
		return false;
	}

	LoadedDataSets.AddUnique(LoadedDataSet);
	UE_LOG(LogLastFPSGameData, Log, TEXT("GameDataSet 로드 완료: %s"), *DataSetId.ToString());
	return true;
}

const FLastFPSDataTableReference* ULastFPSGameDataSubsystem::FindTableReference(const FGameplayTag TableId) const
{
	for (const ULastFPSGameDataSet* DataSet : LoadedDataSets)
	{
		if (DataSet)
		{
			if (const FLastFPSDataTableReference* Reference = DataSet->FindTable(TableId))
			{
				return Reference;
			}
		}
	}
	return nullptr;
}

void ULastFPSGameDataSubsystem::EnsureStartupDataSetLoaded()
{
	if (!AttemptedDataSetIds.Contains(StartupDataSetId))
	{
		LoadDataSet(StartupDataSetId);
	}
}
