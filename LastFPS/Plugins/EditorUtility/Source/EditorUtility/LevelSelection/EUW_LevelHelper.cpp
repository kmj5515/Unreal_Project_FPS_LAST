#include "EUW_LevelHelper.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

FString UEUW_LevelHelper::GetSaveFilePath()
{
    return FPaths::ProjectSavedDir() + TEXT("Config/EditorLevelFavorites.json");
}

void UEUW_LevelHelper::SaveEditorSettings(const FEUW_EditorSettings& Settings)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    // 경로를 저장합니다.
    RootObject->SetStringField(TEXT("searchPath"), Settings.SearchPath);

    // 즐겨찾기 목록을 저장합니다.
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
            // 경로를 불러옵니다.
            RootObject->TryGetStringField(TEXT("searchPath"), Settings.SearchPath);

            // 즐겨찾기 목록을 불러옵니다.
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

    // 지정한 경로 아래의 모든 에셋을 검색합니다.
    AssetRegistryModule.Get().GetAssetsByPath(FName(*ScanPath), AssetData, true);

    for (const FAssetData& Data : AssetData)
    {
        // 월드 에셋만 필터링합니다.
        if (Data.AssetClassPath.GetAssetName() == TEXT("World"))
        {
            FEUW_MapAssetInfo Info;
            Info.MapName = Data.AssetName;

            // 소프트 오브젝트 경로를 사용해 정확한 에셋 경로를 확보합니다.
            Info.PackagePath = Data.GetSoftObjectPath().ToString();

            Info.bIsFavorite = Settings.FavoriteMaps.Contains(Info.MapName);

            MapAssets.Add(Info);

            UE_LOG(LogTemp, Log, TEXT("LastFPS: Found Map '%s' at path '%s'"), *Info.MapName.ToString(), *Info.PackagePath);
        }
    }

    return MapAssets;
}
#endif
