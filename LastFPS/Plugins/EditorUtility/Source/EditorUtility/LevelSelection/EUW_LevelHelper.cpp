#include "EUW_LevelHelper.h"

#if WITH_EDITOR
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"

FString UEUW_LevelHelper::GetSaveFilePath()
{
    return FPaths::ProjectSavedDir() + TEXT("Config/EditorLevelFavorites.json");
}

void UEUW_LevelHelper::SaveEditorSettings(const FEUW_EditorSettings& Settings)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    
    // 경로 저장
    RootObject->SetStringField(TEXT("searchPath"), Settings.SearchPath);

    // 즐겨찾기 저장
    TArray<TSharedPtr<FJsonValue>> FavoritesArray;
    for (const FName& MapName : Settings.FavoriteMaps)
    {
        FavoritesArray.Add(MakeShareable(new FJsonValueString(MapName.ToString())));
    }
    RootObject->SetArrayField(TEXT("favorites"), FavoritesArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    FFileHelper::SaveStringToFile(OutputString, *GetSaveFilePath());
}

FEUW_EditorSettings UEUW_LevelHelper::LoadEditorSettings()
{
    FEUW_EditorSettings Settings;
    FString JsonString;

    if (FFileHelper::LoadFileToString(JsonString, *GetSaveFilePath()))
    {
        TSharedPtr<FJsonObject> RootObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

        if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
        {
            // 경로 로드
            RootObject->TryGetStringField(TEXT("searchPath"), Settings.SearchPath);

            // 즐겨찾기 로드
            const TArray<TSharedPtr<FJsonValue>>* FavoritesArray;
            if (RootObject->TryGetArrayField(TEXT("favorites"), FavoritesArray))
            {
                for (const auto& Value : *FavoritesArray)
                {
                    Settings.FavoriteMaps.Add(FName(*Value->AsString()));
                }
            }
        }
    }

    return Settings;
}

TArray<FEUW_MapAssetInfo> UEUW_LevelHelper::GetMapAssetsInPath(const FString& ScanPath)
{
    TArray<FEUW_MapAssetInfo> MapAssets;
    FEUW_EditorSettings Settings = LoadEditorSettings();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetData;
    
    // 특정 경로 하위의 모든 에셋 검색
    AssetRegistryModule.Get().GetAssetsByPath(FName(*ScanPath), AssetData, true);

    for (const FAssetData& Data : AssetData)
    {
        // UWorld 클래스만 필터링
        if (Data.AssetClassPath.GetAssetName() == TEXT("World"))
        {
            FEUW_MapAssetInfo Info;
            Info.MapName = Data.AssetName;
            
            // PackageName 대신 SoftObjectPath의 문자열을 사용하여 정확한 에셋 경로를 확보합니다.
            Info.PackagePath = Data.GetSoftObjectPath().ToString();
            
            Info.bIsFavorite = Settings.FavoriteMaps.Contains(Info.MapName);
            
            MapAssets.Add(Info);
            
            UE_LOG(LogTemp, Log, TEXT("LastFPS: Found Map '%s' at path '%s'"), *Info.MapName.ToString(), *Info.PackagePath);
        }
    }

    return MapAssets;
}
#endif
