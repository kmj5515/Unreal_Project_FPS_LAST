#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EUW_LevelHelper.generated.h"

#if WITH_EDITOR
USTRUCT(BlueprintType)
struct FEUW_MapAssetInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName MapName;

    UPROPERTY(BlueprintReadOnly)
    FString PackagePath;

    UPROPERTY(BlueprintReadOnly)
    bool bIsFavorite = false;
};

USTRUCT(BlueprintType)
struct FEUW_EditorSettings
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString SearchPath = TEXT("/Game");

    UPROPERTY(BlueprintReadWrite)
    TArray<FName> FavoriteMaps;
};

/** 레벨 선택과 즐겨찾기 설정을 처리하는 에디터 유틸리티 헬퍼입니다. */
UCLASS()
class EDITORUTILITY_API UEUW_LevelHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 설정을 파일로 저장합니다. */
    UFUNCTION(BlueprintCallable, Category = "EUW|Editor")
    static void SaveEditorSettings(const FEUW_EditorSettings& Settings);

    /** 설정을 파일에서 불러옵니다. */
    UFUNCTION(BlueprintCallable, Category = "EUW|Editor")
    static FEUW_EditorSettings LoadEditorSettings();

    /** 지정한 경로의 모든 맵 에셋 정보를 가져옵니다. */
    UFUNCTION(BlueprintCallable, Category = "EUW|Editor")
    static TArray<FEUW_MapAssetInfo> GetMapAssetsInPath(const FString& ScanPath);

private:
    static FString GetSaveFilePath();
};
#endif
