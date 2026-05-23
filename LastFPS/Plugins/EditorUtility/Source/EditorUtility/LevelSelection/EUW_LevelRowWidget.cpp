#include "EUW_LevelRowWidget.h"

#if WITH_EDITOR
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "EUW_LevelHelper.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Subsystems/AssetEditorSubsystem.h"

void UEUW_LevelRowWidget::SetMapInfo(const FEUW_MapAssetInfo& InInfo)
{
    MapInfo = InInfo;

    if (MapNameText)
    {
        MapNameText->SetText(FText::FromName(MapInfo.MapName));
    }

    if (FullPathText)
    {
        FullPathText->SetText(FText::FromString(MapInfo.PackagePath));
    }

    if (FavoriteCheckBox)
    {
        FavoriteCheckBox->SetIsChecked(MapInfo.bIsFavorite);
    }
}

void UEUW_LevelRowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (FavoriteCheckBox)
    {
        FavoriteCheckBox->OnCheckStateChanged.AddDynamic(this, &UEUW_LevelRowWidget::HandleFavoriteChanged);
    }

    if (OpenLevelButton)
    {
        OpenLevelButton->OnClicked.AddDynamic(this, &UEUW_LevelRowWidget::HandleOpenLevelClicked);
    }
}

void UEUW_LevelRowWidget::HandleFavoriteChanged(bool bIsChecked)
{
    FEUW_EditorSettings Settings = UEUW_LevelHelper::LoadEditorSettings();

    if (bIsChecked)
    {
        Settings.FavoriteMaps.AddUnique(MapInfo.MapName);
    }
    else
    {
        Settings.FavoriteMaps.Remove(MapInfo.MapName);
    }
    
    UEUW_LevelHelper::SaveEditorSettings(Settings);
}

void UEUW_LevelRowWidget::HandleOpenLevelClicked()
{
    if (!GEditor || MapInfo.PackagePath.IsEmpty())
    {
        return;
    }

    FString PathToLoad = MapInfo.PackagePath;

    // 💡 크래시 방지: 한 프레임 뒤에 실행하여 UI 이벤트를 안전하게 종료시킵니다.
    GEditor->GetTimerManager()->SetTimerForNextTick([PathToLoad]()
    {
        // 1. 현재 맵 변경사항 저장 확인 (사용자가 취소하면 false 반환)
        if (FEditorFileUtils::SaveDirtyPackages(true, true, true))
        {
            // 2. 레벨 전환을 위한 전용 유틸리티 사용
            // 맵(World) 자산은 일반 자산 에디터가 아닌 전용 로드 로직을 타야 안전합니다.
            UE_LOG(LogTemp, Log, TEXT("LastFPS: Loading Level: %s"), *PathToLoad);
            FEditorFileUtils::LoadMap(PathToLoad);
        }
    });
}
#endif
