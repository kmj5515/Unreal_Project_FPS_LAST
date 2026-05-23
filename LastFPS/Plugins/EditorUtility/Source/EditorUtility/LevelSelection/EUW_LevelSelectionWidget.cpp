#include "EUW_LevelSelectionWidget.h"

#if WITH_EDITOR
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "EUW_LevelRowWidget.h"
#include "EUW_LevelHelper.h"
#include "Components/ScrollBoxSlot.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
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

    // 1. 리스트 분리 및 필터링
    TArray<FEUW_MapAssetInfo> FavoriteMaps;
    TArray<FEUW_MapAssetInfo> NormalMaps;

    for (const FEUW_MapAssetInfo& Info : MapInfos)
    {
        if (Info.bIsFavorite)
        {
            FavoriteMaps.Add(Info);
        }
        else if (MapDisplayMode == EMapDisplayMode::All) // 필터링: All 모드일 때만 일반 맵 추가
        {
            NormalMaps.Add(Info);
        }
    }

    // 2. 각각 이름순 정렬
    auto SortFunc = [](const FEUW_MapAssetInfo& A, const FEUW_MapAssetInfo& B) {
        return A.MapName.ToString() < B.MapName.ToString();
    };
    FavoriteMaps.Sort(SortFunc);
    NormalMaps.Sort(SortFunc);

    // 3. 위젯 추가 헬퍼 람다
    auto AddMapRows = [this](const TArray<FEUW_MapAssetInfo>& Items) {
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

    // ── 즐겨찾기 섹션 ──
    AddMapRows(FavoriteMaps);

    // ── 일반 목록 섹션 (All 모드일 때만 데이터가 있음) ──
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
        // 현재 경로를 OS 절대 경로로 변환
        FString CurrentPath = PathTextBox ? PathTextBox->GetText().ToString() : TEXT("/Game");
        FString AbsPath;
        FPackageName::TryConvertLongPackageNameToFilename(CurrentPath, AbsPath);
        
        if (AbsPath.IsEmpty())
        {
            AbsPath = FPaths::ProjectContentDir();
        }

        FString ChosenFolder;
        const FString Title = TEXT("Select Level Folder");
        
        // OS 표준 폴더 선택 다이얼로그 호출
        bool bSelected = DesktopPlatform->OpenDirectoryDialog(
            nullptr, // ParentWindowHandle (nullptr 사용 시 메인 윈도우가 부모가 됨)
            Title,
            AbsPath,
            ChosenFolder
        );

        if (bSelected)
        {
            // 선택된 절대 경로를 다시 언리얼 상대 경로(/Game/...)로 변환
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
