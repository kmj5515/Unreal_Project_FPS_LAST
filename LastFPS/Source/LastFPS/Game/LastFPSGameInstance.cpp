#include "Game/LastFPSGameInstance.h"

#include "UI/LastFPSLoadingScreenWidget.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

void ULastFPSGameInstance::Init()
{
    Super::Init();

    PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ULastFPSGameInstance::HandlePreLoadMap);
    PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ULastFPSGameInstance::HandlePostLoadMap);
}

void ULastFPSGameInstance::Shutdown()
{
    if (PreLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
        PreLoadMapHandle.Reset();
    }

    if (PostLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
        PostLoadMapHandle.Reset();
    }

    HideLoadingScreenWidget();
    Super::Shutdown();
}

void ULastFPSGameInstance::SaveSelectedCharacterIndex(const FString& PlayerKey, int32 SelectedIndex)
{
    if (PlayerKey.IsEmpty())
    {
        return;
    }

    SelectedCharacterIndexByPlayerKey.Add(PlayerKey, FMath::Max(0, SelectedIndex));
}

bool ULastFPSGameInstance::TryGetSelectedCharacterIndex(const FString& PlayerKey, int32& OutSelectedIndex) const
{
    if (const int32* FoundIndex = SelectedCharacterIndexByPlayerKey.Find(PlayerKey))
    {
        OutSelectedIndex = *FoundIndex;
        return true;
    }

    return false;
}

void ULastFPSGameInstance::RequestTravelToMatch(const FString& MatchMapURL)
{
    BeginServerTravel(MatchMapURL, ELastFPSTravelDirection::LobbyToMatch);
}

void ULastFPSGameInstance::RequestTravelToLobby(const FString& LobbyMapURL)
{
    BeginServerTravel(LobbyMapURL, ELastFPSTravelDirection::MatchToLobby);
}

static bool ShouldShowLoadingScreenForWorld(const UWorld* World)
{
    return World && World->GetNetMode() != NM_DedicatedServer;
}

void ULastFPSGameInstance::BeginServerTravel(const FString& MapURL, ELastFPSTravelDirection Direction)
{
    if (MapURL.IsEmpty())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    ActiveTravelDirection = Direction;
    PendingMapDisplayName = ExtractShortMapName(MapURL);
    bLoadingScreenActive = ShouldShowLoadingScreenForWorld(World);
    bMapLoadingPhase = false;
    LoadingScreenHideRetryCount = 0;
    LoadingScreenShowStartSeconds = FPlatformTime::Seconds();

    if (bLoadingScreenActive)
    {
        ShowLoadingScreenWidget();
    }

    World->ServerTravel(MapURL);
}

void ULastFPSGameInstance::HandlePreLoadMap(const FString& MapName)
{
    if (!bLoadingScreenActive)
    {
        return;
    }

    bMapLoadingPhase = true;
    PendingMapDisplayName = ExtractShortMapName(MapName);
    HideLoadingScreenWidget();
}

void ULastFPSGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (!bLoadingScreenActive || !LoadedWorld)
    {
        return;
    }

    ShowLoadingScreenWidget();
    RefreshLoadingScreenContent();
    ScheduleFinalizeLoadingScreen(0.1f);
}

void ULastFPSGameInstance::ShowLoadingScreenWidget()
{
    if (!LoadingScreenWidgetClass)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!ShouldShowLoadingScreenForWorld(World))
    {
        return;
    }

    if (LoadingScreenWidget && LoadingScreenWidget->IsInViewport())
    {
        RefreshLoadingScreenContent();
        return;
    }

    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        LoadingScreenWidget = CreateWidget<ULastFPSLoadingScreenWidget>(PC, LoadingScreenWidgetClass);
    }
    else
    {
        LoadingScreenWidget = CreateWidget<ULastFPSLoadingScreenWidget>(this, LoadingScreenWidgetClass);
    }
    if (!LoadingScreenWidget)
    {
        return;
    }

    LoadingScreenWidget->SetIsFocusable(false);
    LoadingScreenWidget->AddToViewport(LoadingScreenZOrder);
    RefreshLoadingScreenContent();
}

void ULastFPSGameInstance::HideLoadingScreenWidget()
{
    if (LoadingScreenWidget)
    {
        LoadingScreenWidget->RemoveFromParent();
        LoadingScreenWidget = nullptr;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FinalizeLoadingTimerHandle);
    }
}

void ULastFPSGameInstance::RefreshLoadingScreenContent()
{
    if (!LoadingScreenWidget)
    {
        return;
    }

    const FText StatusText = BuildStatusText(ActiveTravelDirection, bMapLoadingPhase);
    const FText MapNameText = PendingMapDisplayName.IsEmpty()
        ? FText::GetEmpty()
        : FText::FromString(PendingMapDisplayName);

    LoadingScreenWidget->SetStatusText(StatusText);
    LoadingScreenWidget->SetMapNameText(MapNameText);
    LoadingScreenWidget->OnLoadingScreenUpdated(StatusText, MapNameText);
}

void ULastFPSGameInstance::ScheduleFinalizeLoadingScreen(float DelaySeconds)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FinalizeLoadingTimerHandle,
            this,
            &ULastFPSGameInstance::TryFinalizeLoadingScreen,
            FMath::Max(0.01f, DelaySeconds),
            false);
    }
}

void ULastFPSGameInstance::TryFinalizeLoadingScreen()
{
    if (!bLoadingScreenActive)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const double Elapsed = FPlatformTime::Seconds() - LoadingScreenShowStartSeconds;
    if (Elapsed < static_cast<double>(MinLoadingScreenDisplaySeconds))
    {
        const float Remaining = static_cast<float>(MinLoadingScreenDisplaySeconds - Elapsed);
        ScheduleFinalizeLoadingScreen(Remaining);
        return;
    }

    if (!World->GetFirstPlayerController() && LoadingScreenHideRetryCount < LoadingScreenHideMaxRetries)
    {
        ++LoadingScreenHideRetryCount;
        ScheduleFinalizeLoadingScreen(0.1f);
        return;
    }

    bLoadingScreenActive = false;
    bMapLoadingPhase = false;
    ActiveTravelDirection = ELastFPSTravelDirection::None;
    PendingMapDisplayName.Empty();
    HideLoadingScreenWidget();
}

FString ULastFPSGameInstance::ExtractShortMapName(const FString& MapURL)
{
    FString Path = MapURL;
    int32 QueryIdx = INDEX_NONE;
    if (Path.FindChar(TEXT('?'), QueryIdx))
    {
        Path.LeftInline(QueryIdx);
    }

    int32 DotIdx = INDEX_NONE;
    if (Path.FindChar(TEXT('.'), DotIdx))
    {
        Path.LeftInline(DotIdx);
    }

    const FString BaseName = FPaths::GetBaseFilename(Path);
    return BaseName.IsEmpty() ? Path : BaseName;
}

FText ULastFPSGameInstance::BuildStatusText(ELastFPSTravelDirection Direction, bool bMapLoadingPhase)
{
    if (bMapLoadingPhase)
    {
        return NSLOCTEXT("LastFPS", "LoadingStatus_MapLoading", "맵 로딩 중...");
    }

    switch (Direction)
    {
    case ELastFPSTravelDirection::LobbyToMatch:
        return NSLOCTEXT("LastFPS", "LoadingStatus_ToMatch", "매치로 이동 중...");
    case ELastFPSTravelDirection::MatchToLobby:
        return NSLOCTEXT("LastFPS", "LoadingStatus_ToLobby", "로비로 돌아가는 중...");
    default:
        return NSLOCTEXT("LastFPS", "LoadingStatus_Generic", "이동 중...");
    }
}
