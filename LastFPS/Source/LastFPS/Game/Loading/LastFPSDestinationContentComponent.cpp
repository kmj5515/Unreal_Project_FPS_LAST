#include "Game/Loading/LastFPSDestinationContentComponent.h"

#include "Data/Definitions/LastFPSDestinationContentSet.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSDestinationContent, Log, All);

namespace LastFPSDestinationContentDebug
{
#if !UE_BUILD_SHIPPING
    static bool bShowOnScreen = false;
    static FAutoConsoleVariableRef CVarShowOnScreen(
        TEXT("LastFPS.Loading.ShowDebug"),
        bShowOnScreen,
        TEXT("목적지 콘텐츠 게이트의 진행 상황을 화면에 노란색으로 표시한다."),
        ECVF_Default);
#endif

    // 로딩 화면이 걷힌 뒤에도 읽을 수 있도록 넉넉히 유지한다.
    static constexpr float DisplaySeconds = 10.0f;

    static void Print(const FString& Message)
    {
#if !UE_BUILD_SHIPPING
        if (bShowOnScreen && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, DisplaySeconds, FColor::Yellow, Message);
        }
#endif
    }
}

namespace LastFPSDestinationContentInternal
{
    /**
     * 이미 메모리에 있는 에셋은 목록에 있어도 로딩 화면을 붙잡지 못한다.
     * 게이트가 헛도는지 판별하는 유일한 신호라 요약과 함께 남긴다.
     */
    static int32 LogPathBreakdown(const TArray<FSoftObjectPath>& Paths, const UObject* ContentSet)
    {
        int32 AlreadyLoaded = 0;
        for (const FSoftObjectPath& Path : Paths)
        {
            const bool bResolved = Path.ResolveObject() != nullptr;
            AlreadyLoaded += bResolved ? 1 : 0;

            UE_LOG(LogLastFPSDestinationContent, Verbose, TEXT("  [%s] %s"),
                bResolved ? TEXT("이미 로드") : TEXT("스트리밍"),
                *Path.ToString());

            // 화면에는 게이트가 실제로 붙잡은 것만 — 이미 로드된 항목까지 띄우면 묻힌다.
            if (!bResolved)
            {
                LastFPSDestinationContentDebug::Print(
                    FString::Printf(TEXT("      └ %s"), *Path.GetAssetName()));
            }
        }

        const int32 ToStream = Paths.Num() - AlreadyLoaded;
        UE_LOG(LogLastFPSDestinationContent, Log,
            TEXT("%s: 총 %d개 중 이미 로드 %d개 / 실제 스트리밍 %d개"),
            *GetNameSafe(ContentSet), Paths.Num(), AlreadyLoaded, ToStream);

        LastFPSDestinationContentDebug::Print(FString::Printf(
            TEXT("[로딩] %s — 총 %d개 / 이미 로드 %d개 / 스트리밍 %d개"),
            *GetNameSafe(ContentSet), Paths.Num(), AlreadyLoaded, ToStream));

        if (ToStream == 0 && Paths.Num() > 0)
        {
            UE_LOG(LogLastFPSDestinationContent, Warning,
                TEXT("모든 항목이 이미 로드되어 게이트가 로딩 화면을 붙잡지 않습니다. "
                     "해당 에셋들이 시작 시점에 강한 참조로 로드되고 있는지 확인하세요."));

            LastFPSDestinationContentDebug::Print(
                TEXT("[로딩] 경고: 전부 이미 로드됨 — 게이트가 화면을 붙잡지 않음"));
        }

        return ToStream;
    }
}

ULastFPSDestinationContentComponent::ULastFPSDestinationContentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool ULastFPSDestinationContentComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
    // Unloaded(StartContentLoad 이전)에서 true 를 반환하면 로드할 것이 없는 맵이 영구 대기한다.
    if (LoadState != EContentLoadState::Loading)
    {
        return false;
    }

    OutReason = TEXT("LastFPS destination content is still loading");
    return true;
}

void ULastFPSDestinationContentComponent::StartContentLoad(const ULastFPSDestinationContentSet* ContentSet)
{
    if (LoadState != EContentLoadState::Unloaded)
    {
        UE_LOG(LogLastFPSDestinationContent, Warning,
            TEXT("콘텐츠 로드가 이미 시작된 뒤 다시 요청되었습니다. 요청을 무시합니다."));
        return;
    }

    TArray<FSoftObjectPath> RequiredPaths;
    if (ContentSet)
    {
        ContentSet->CollectRequiredPaths(RequiredPaths);
    }

    if (RequiredPaths.IsEmpty())
    {
        // ContentSet 미지정은 정상 구성이다(로딩 게이트가 필요 없는 맵).
        FinishLoad();
        return;
    }

    LastFPSDestinationContentInternal::LogPathBreakdown(RequiredPaths, ContentSet);

    LoadStartSeconds = FPlatformTime::Seconds();
    LoadState = EContentLoadState::Loading;
    LoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        RequiredPaths,
        FStreamableDelegate::CreateUObject(this, &ThisClass::HandleLoadCompleted));

    if (!LoadHandle.IsValid())
    {
        UE_LOG(LogLastFPSDestinationContent, Error,
            TEXT("콘텐츠 %d개 비동기 로드 요청에 실패했습니다: %s"),
            RequiredPaths.Num(),
            *GetNameSafe(ContentSet));
        FinishLoad();
        return;
    }
}

void ULastFPSDestinationContentComponent::HandleLoadCompleted()
{
    const double ElapsedMs = (FPlatformTime::Seconds() - LoadStartSeconds) * 1000.0;

    // 로드에 걸린 시간이 곧 로딩 화면이 추가로 유지된 시간이다.
    UE_LOG(LogLastFPSDestinationContent, Log,
        TEXT("목적지 콘텐츠 로드 완료 — %.1fms 동안 로딩 화면 유지"), ElapsedMs);

    LastFPSDestinationContentDebug::Print(FString::Printf(
        TEXT("[로딩] 완료 — %.1fms 동안 화면 유지"), ElapsedMs));

    if (LoadHandle.IsValid())
    {
        TArray<UObject*> LoadedAssets;
        LoadHandle->GetLoadedAssets(LoadedAssets);
        UE_LOG(LogLastFPSDestinationContent, Verbose,
            TEXT("로드된 에셋 %d개"), LoadedAssets.Num());
    }

    FinishLoad();
}

void ULastFPSDestinationContentComponent::FinishLoad()
{
    LoadState = EContentLoadState::Ready;
    OnContentReady.Broadcast();
}

void ULastFPSDestinationContentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    OnContentReady.Clear();

    if (LoadHandle.IsValid())
    {
        // 목적지를 떠나면 이 맵 전용 콘텐츠의 상주를 해제한다.
        LoadHandle->CancelHandle();
        LoadHandle.Reset();
    }

    Super::EndPlay(EndPlayReason);
}
