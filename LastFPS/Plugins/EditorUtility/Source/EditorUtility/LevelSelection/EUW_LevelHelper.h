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

/**
 * 에디터 유틸리티 툴을 위한 레벨 선택 및 즐겨찾기 헬퍼 클래스
 */
UCLASS()
class EDITORUTILITY_API UEUW_LevelHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 설정(즐겨찾기 + 경로)을 JSON으로 저장 */
    UFUNCTION(BlueprintCallable, Category = "EUW|Editor")
    static void SaveEditorSettings(const FEUW_EditorSettings& Settings);

    /** 설정을 JSON에서 로드 */
    UFUNCTION(BlueprintCallable, Category = "EUW|Editor")
    static FEUW_EditorSettings LoadEditorSettings();

    /** 특정 경로 내의 모든 맵 에셋 정보를 가져옴 */
    UFUNCTION(BlueprintCallable, Category = "EUW|Editor")
    static TArray<FEUW_MapAssetInfo> GetMapAssetsInPath(const FString& ScanPath);

private:
    static FString GetSaveFilePath();
};
#endif
