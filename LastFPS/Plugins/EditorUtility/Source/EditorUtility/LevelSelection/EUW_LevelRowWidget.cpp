#include "EUW_LevelRowWidget.h"

#if WITH_EDITOR
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EUW_LevelHelper.h"
#include "FileHelpers.h"
#include "GameMapsSettings.h"
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

    if (SetAsStartLevelButton)
    {
        SetAsStartLevelButton->OnClicked.AddDynamic(this, &UEUW_LevelRowWidget::HandleSetAsStartLevelClicked);
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

void UEUW_LevelRowWidget::HandleSetAsStartLevelClicked()
{
    if (MapInfo.PackagePath.IsEmpty())
    {
        return;
    }

    UGameMapsSettings* Settings = GetMutableDefault<UGameMapsSettings>();
    if (!Settings)
    {
        return;
    }

    FProperty* Prop = FindFProperty<FProperty>(
        UGameMapsSettings::StaticClass(),
        GET_MEMBER_NAME_CHECKED(UGameMapsSettings, EditorStartupMap));

    Settings->PreEditChange(Prop);
    Settings->EditorStartupMap = FSoftObjectPath(MapInfo.PackagePath);
    FPropertyChangedEvent ChangeEvent(Prop);
    Settings->PostEditChangeProperty(ChangeEvent);

    Settings->TryUpdateDefaultConfigFile();

    UE_LOG(LogTemp, Log, TEXT("LastFPS: Editor Startup Map set to %s"), *MapInfo.PackagePath);
}

void UEUW_LevelRowWidget::HandleOpenLevelClicked()
{
    if (!GEditor || MapInfo.PackagePath.IsEmpty())
    {
        return;
    }

    FString PathToLoad = MapInfo.PackagePath;

    // 다음 프레임에 실행하여 사용자 인터페이스 이벤트를 안전하게 마무리합니다.
    GEditor->GetTimerManager()->SetTimerForNextTick([PathToLoad]()
    {
        // 현재 맵 변경 사항 저장 여부를 확인합니다.
        if (FEditorFileUtils::SaveDirtyPackages(true, true, true))
        {
            // 레벨 전환 전용 로드 함수를 사용합니다.
            // 월드 자산은 일반 자산 편집기가 아닌 전용 로드 경로를 사용해야 안전합니다.
            UE_LOG(LogTemp, Log, TEXT("LastFPS: Loading Level: %s"), *PathToLoad);
            FEditorFileUtils::LoadMap(PathToLoad);
        }
    });
}
#endif
