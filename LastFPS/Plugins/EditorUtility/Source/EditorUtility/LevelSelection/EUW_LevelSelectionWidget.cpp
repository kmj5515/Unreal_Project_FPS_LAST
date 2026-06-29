#include "EUW_LevelSelectionWidget.h"

#if WITH_EDITOR
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "EUW_LevelHelper.h"
#include "EUW_LevelRowWidget.h"
#include "Framework/Application/SlateApplication.h"

void UEUW_LevelSelectionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (RefreshButton)
    {
        RefreshButton->OnClicked.AddDynamic(this, &UEUW_LevelSelectionWidget::HandleRefreshClicked);
    }

    if (BrowseButton)
    {
        BrowseButton->OnClicked.AddDynamic(this, &UEUW_LevelSelectionWidget::HandleBrowseClicked);
    }

    if (AllButton)
    {
        AllButton->OnClicked.AddDynamic(this, &UEUW_LevelSelectionWidget::HandleAllButtonClicked);
    }

    if (FavoriteButton)
    {
        FavoriteButton->OnClicked.AddDynamic(this, &UEUW_LevelSelectionWidget::HandleFavoriteButtonClicked);
    }

    if (PathTextBox)
    {
        PathTextBox->OnTextCommitted.AddDynamic(this, &UEUW_LevelSelectionWidget::HandlePathCommitted);

        FEUW_EditorSettings Settings = UEUW_LevelHelper::LoadEditorSettings();
        PathTextBox->SetText(FText::FromString(Settings.SearchPath));
    }

    ChangeDisplayMode();
}

void UEUW_LevelSelectionWidget::RefreshMapList()
{
    if (!MapScrollBox || !RowWidgetClass)
    {
        return;
    }

    MapScrollBox->ClearChildren();

    FString CurrentPath = PathTextBox ? PathTextBox->GetText().ToString() : TEXT("/Game");
    TArray<FEUW_MapAssetInfo> MapInfos = UEUW_LevelHelper::GetMapAssetsInPath(CurrentPath);

    // 즐겨찾기와 일반 목록을 분리합니다.
    TArray<FEUW_MapAssetInfo> FavoriteMaps;
    TArray<FEUW_MapAssetInfo> NormalMaps;

    for (const FEUW_MapAssetInfo& Info : MapInfos)
    {
        if (Info.bIsFavorite)
        {
            FavoriteMaps.Add(Info);
        }
        else if (MapDisplayMode == EMapDisplayMode::All)
        {
            NormalMaps.Add(Info);
        }
    }

    // 이름순으로 정렬합니다.
    auto SortFunc = [](const FEUW_MapAssetInfo& A, const FEUW_MapAssetInfo& B)
    {
        return A.MapName.ToString() < B.MapName.ToString();
    };
    FavoriteMaps.Sort(SortFunc);
    NormalMaps.Sort(SortFunc);

    // 맵 행 위젯을 추가합니다.
    auto AddMapRows = [this](const TArray<FEUW_MapAssetInfo>& Items)
    {
        for (const FEUW_MapAssetInfo& Info : Items)
        {
            if (UEUW_LevelRowWidget* RowWidget = CreateWidget<UEUW_LevelRowWidget>(this, RowWidgetClass))
            {
                RowWidget->SetMapInfo(Info);

                if (UPanelSlot* PanelSlot = MapScrollBox->AddChild(RowWidget))
                {
                    if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(PanelSlot))
                    {
                        ScrollSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
                        ScrollSlot->SetHorizontalAlignment(HAlign_Fill);
                    }
                }
            }
        }
    };

    AddMapRows(FavoriteMaps);
    AddMapRows(NormalMaps);
}

void UEUW_LevelSelectionWidget::ChangeDisplayMode()
{
    if (AllButton)
    {
        AllButton->SetIsEnabled(MapDisplayMode != EMapDisplayMode::All);
    }

    if (FavoriteButton)
    {
        FavoriteButton->SetIsEnabled(MapDisplayMode != EMapDisplayMode::Favorite);
    }

    RefreshMapList();
}

void UEUW_LevelSelectionWidget::HandleRefreshClicked()
{
    RefreshMapList();
}

void UEUW_LevelSelectionWidget::HandleBrowseClicked()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        // 현재 경로를 운영체제 절대 경로로 변환합니다.
        FString CurrentPath = PathTextBox ? PathTextBox->GetText().ToString() : TEXT("/Game");
        FString AbsPath;
        FPackageName::TryConvertLongPackageNameToFilename(CurrentPath, AbsPath);

        if (AbsPath.IsEmpty())
        {
            AbsPath = FPaths::ProjectContentDir();
        }

        FString ChosenFolder;
        const FString Title = TEXT("Select Level Folder");

        // 운영체제 폴더 선택 창을 엽니다.
        bool bSelected = DesktopPlatform->OpenDirectoryDialog(
            nullptr,
            Title,
            AbsPath,
            ChosenFolder
        );

        if (bSelected)
        {
            // 선택한 절대 경로를 언리얼 패키지 경로로 변환합니다.
            FString PackagePath;
            if (FPackageName::TryConvertFilenameToLongPackageName(ChosenFolder, PackagePath))
            {
                if (PathTextBox)
                {
                    PathTextBox->SetText(FText::FromString(PackagePath));
                    HandlePathCommitted(PathTextBox->GetText(), ETextCommit::Default);
                }
            }
        }
    }
}

void UEUW_LevelSelectionWidget::HandlePathCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    FEUW_EditorSettings Settings = UEUW_LevelHelper::LoadEditorSettings();
    Settings.SearchPath = Text.ToString();
    UEUW_LevelHelper::SaveEditorSettings(Settings);

    RefreshMapList();
}

void UEUW_LevelSelectionWidget::HandleAllButtonClicked()
{
    MapDisplayMode = EMapDisplayMode::All;
    ChangeDisplayMode();
}

void UEUW_LevelSelectionWidget::HandleFavoriteButtonClicked()
{
    MapDisplayMode = EMapDisplayMode::Favorite;
    ChangeDisplayMode();
}
#endif
