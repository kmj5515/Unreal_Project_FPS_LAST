#include "Game/Loading/LastFPSDestinationContentComponent.h"

#include "Data/Definitions/LastFPSDestinationContentSet.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "NativeGameplayTags.h"
#include "PipelineStateCache.h"
#include "ShaderCompiler.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSDestinationContent, Log, All);

namespace LastFPSDestinationContentProgress
{
    UE_DEFINE_GAMEPLAY_TAG_STATIC(Content, "Loading.Process.Destination.Content");

    static FLastFPSDestinationLoadingProgressSettings ResolveSettings(
        const ULastFPSDestinationContentSet* ContentSet)
    {
        FLastFPSDestinationLoadingProgressSettings Settings = ContentSet
            ? ContentSet->LoadingProgress
            : FLastFPSDestinationLoadingProgressSettings();

        Settings.OverallProgressShare =
            FMath::Clamp(Settings.OverallProgressShare, 0.05f, 0.95f);
        Settings.AssetAndGameplayCueWeight =
            FMath::Max(Settings.AssetAndGameplayCueWeight, 0.0f);
        Settings.ActorPoolWeight =
            FMath::Max(Settings.ActorPoolWeight, 0.0f);
        Settings.RenderComponentWeight =
            FMath::Max(Settings.RenderComponentWeight, 0.0f);
        Settings.ShaderAndPSOWeight =
            FMath::Max(Settings.ShaderAndPSOWeight, 0.0f);

        const float TotalStageWeight =
            Settings.AssetAndGameplayCueWeight
            + Settings.ActorPoolWeight
            + Settings.RenderComponentWeight
            + Settings.ShaderAndPSOWeight;
        if (TotalStageWeight <= KINDA_SMALL_NUMBER)
        {
            Settings = FLastFPSDestinationLoadingProgressSettings();
        }
        return Settings;
    }
}

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
    if (LoadState == EContentLoadState::Unloaded || LoadState == EContentLoadState::Ready)
    {
        return false;
    }

    OutReason = LoadState == EContentLoadState::WarmingRender
        ? TEXT("LastFPS player render components are warming up")
        : TEXT("LastFPS destination content is still loading");
    return true;
}

void ULastFPSDestinationContentComponent::StartContentLoad(
    ULastFPSDestinationContentSet* ContentSet)
{
    if (LoadState != EContentLoadState::Unloaded)
    {
        UE_LOG(LogLastFPSDestinationContent, Warning,
            TEXT("콘텐츠 로드가 이미 시작된 뒤 다시 요청되었습니다. 요청을 무시합니다."));
        return;
    }

    ActiveContentSet = ContentSet;
    LoadStartSeconds = FPlatformTime::Seconds();
    RegisterLoadingProgress();
    BeginNextLoadPhase();
}

void ULastFPSDestinationContentComponent::BeginNextLoadPhase()
{
    TArray<FSoftObjectPath> RequiredPaths;
    if (ActiveContentSet)
    {
        ActiveContentSet->CollectRequiredPaths(RequiredPaths);
    }

    TArray<FSoftObjectPath> NewPaths;
    for (const FSoftObjectPath& Path : RequiredPaths)
    {
        if (Path.IsValid() && !RequestedPaths.Contains(Path))
        {
            RequestedPaths.Add(Path);
            NewPaths.Add(Path);
        }
    }

    if (NewPaths.IsEmpty())
    {
        HandleAssetsLoaded();
        return;
    }

    ++LoadPhase;
    LastFPSDestinationContentInternal::LogPathBreakdown(
        NewPaths,
        ActiveContentSet);

    LoadState = EContentLoadState::LoadingAssets;
    TSharedPtr<FStreamableHandle> PhaseHandle =
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            NewPaths,
            FStreamableDelegate::CreateUObject(
                this,
                &ThisClass::HandleLoadPhaseCompleted));

    if (!PhaseHandle.IsValid())
    {
        UE_LOG(LogLastFPSDestinationContent, Error,
            TEXT("콘텐츠 %d개 비동기 로드 요청에 실패했습니다: %s"),
            NewPaths.Num(),
            *GetNameSafe(ActiveContentSet));
        HandleAssetsLoaded();
        return;
    }

    LoadHandles.Add(MoveTemp(PhaseHandle));
    if (UWorld* World = GetWorld();
        World && !World->GetTimerManager().IsTimerActive(AssetProgressTimerHandle))
    {
        AssetProgressTimerHandle = World->GetTimerManager().SetTimerForNextTick(
            this,
            &ThisClass::PollAssetLoadProgress);
    }
}

void ULastFPSDestinationContentComponent::PollAssetLoadProgress()
{
    if (LoadState != EContentLoadState::LoadingAssets)
    {
        return;
    }

    int32 LoadedPathCount = 0;
    for (const FSoftObjectPath& Path : RequestedPaths)
    {
        LoadedPathCount += Path.ResolveObject() ? 1 : 0;
    }

    float HandleProgress = 0.0f;
    if (!LoadHandles.IsEmpty() && LoadHandles.Last().IsValid())
    {
        HandleProgress = LoadHandles.Last()->GetProgress();
    }

    const float ResolvedPathProgress = RequestedPaths.IsEmpty()
        ? 0.0f
        : static_cast<float>(LoadedPathCount) / RequestedPaths.Num();

    // 후속 단계에서 의존성이 추가될 수 있으므로 최종 완료 신호 전에는 100%를 예약한다.
    const float MeasuredProgress =
        FMath::Min(FMath::Max(ResolvedPathProgress, HandleProgress), 0.95f);
    AssetStageProgress = FMath::Max(AssetStageProgress, MeasuredProgress);
    UpdateLoadingProgress();

    if (UWorld* World = GetWorld())
    {
        AssetProgressTimerHandle = World->GetTimerManager().SetTimerForNextTick(
            this,
            &ThisClass::PollAssetLoadProgress);
    }
}

void ULastFPSDestinationContentComponent::HandleLoadPhaseCompleted()
{
    UE_LOG(LogLastFPSDestinationContent, Log,
        TEXT("목적지 콘텐츠 로드 단계 %d 완료"), LoadPhase);

    // Data Table처럼 로드된 뒤에야 내부 소프트 참조를 열거할 수 있는 계약을 다시 평가한다.
    BeginNextLoadPhase();
}

void ULastFPSDestinationContentComponent::HandleAssetsLoaded()
{
    if (LoadState == EContentLoadState::AwaitingRenderWarmup
        || LoadState == EContentLoadState::WarmingRender
        || LoadState == EContentLoadState::Ready)
    {
        return;
    }

    LoadState = EContentLoadState::AwaitingRenderWarmup;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AssetProgressTimerHandle);
    }
    AssetStageProgress = 1.0f;
    UpdateLoadingProgress();

    UE_LOG(LogLastFPSDestinationContent, Log,
        TEXT("목적지 에셋 로드 완료 — 실제 Pawn 생성 및 렌더 컴포넌트 준비 요청"));

    OnAssetsLoaded.Broadcast();

    // 구독자가 없거나 렌더 준비를 시작하지 않은 맵은 기존처럼 에셋 로드만으로 완료한다.
    if (LoadState == EContentLoadState::AwaitingRenderWarmup)
    {
        FinishLoad();
    }
}

void ULastFPSDestinationContentComponent::BeginActorPoolPreparation()
{
    PoolStageProgress = 0.0f;
    UpdateLoadingProgress();
}

void ULastFPSDestinationContentComponent::CompleteActorPoolPreparation()
{
    PoolStageProgress = 1.0f;
    UpdateLoadingProgress();
}

void ULastFPSDestinationContentComponent::BeginRenderWarmup(
    const TArray<AActor*>& Actors,
    FSimpleDelegate OnWarmupCompleted)
{
    if (LoadState != EContentLoadState::AwaitingRenderWarmup)
    {
        UE_LOG(LogLastFPSDestinationContent, Warning,
            TEXT("렌더 준비를 시작할 수 없는 상태입니다: %d"), static_cast<int32>(LoadState));
        return;
    }

    CompleteActorPoolPreparation();
    RenderWarmupCompletedDelegate = MoveTemp(OnWarmupCompleted);
    WarmupComponents.Reset();
    MaxObservedShaderWork = 0;

    for (AActor* Actor : Actors)
    {
        if (!IsValid(Actor))
        {
            continue;
        }

        TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Actor);
        for (UPrimitiveComponent* Primitive : PrimitiveComponents)
        {
            if (!IsValid(Primitive) || !Primitive->IsRegistered())
            {
                continue;
            }

            WarmupComponents.Add(Primitive);
            Primitive->PrecachePSOs();
        }
    }

    const FLastFPSRenderWarmupSettings Settings =
        ActiveContentSet ? ActiveContentSet->RenderWarmup : FLastFPSRenderWarmupSettings();

    if (!Settings.bEnabled || !FApp::CanEverRender() || WarmupComponents.IsEmpty())
    {
        UE_LOG(LogLastFPSDestinationContent, Log,
            TEXT("렌더 준비 생략 — 활성=%s, 렌더 가능=%s, 컴포넌트=%d"),
            Settings.bEnabled ? TEXT("예") : TEXT("아니요"),
            FApp::CanEverRender() ? TEXT("예") : TEXT("아니요"),
            WarmupComponents.Num());
        RenderStageProgress = 1.0f;
        ShaderStageProgress = 1.0f;
        UpdateLoadingProgress();
        CompleteRenderWarmup();
        return;
    }

    LoadState = EContentLoadState::WarmingRender;
    WarmupStartSeconds = FPlatformTime::Seconds();
    StableRenderFrames = 0;

    // 이 시점 이전에 등록된 SceneProxy 명령이 렌더 스레드에 반영됐는지도 함께 확인한다.
    RenderRegistrationFence.BeginFence();

    UE_LOG(LogLastFPSDestinationContent, Log,
        TEXT("플레이어 렌더 준비 시작 — PrimitiveComponent %d개"),
        WarmupComponents.Num());

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick(
            this, &ThisClass::PollRenderWarmup);
    }
    else
    {
        CompleteRenderWarmup();
    }
}

bool ULastFPSDestinationContentComponent::IsRenderWarmupBusy(
    int32& OutCompilingComponents,
    int32& OutShaderJobs,
    uint32& OutPSORequests) const
{
    OutCompilingComponents = 0;
    for (const TWeakObjectPtr<UPrimitiveComponent>& ComponentPtr : WarmupComponents)
    {
        const UPrimitiveComponent* Component = ComponentPtr.Get();
        if (Component && (!Component->IsRegistered()
            || !Component->IsRenderStateCreated()
            || Component->IsCompiling()))
        {
            ++OutCompilingComponents;
        }
    }

    OutShaderJobs = GShaderCompilingManager
        ? GShaderCompilingManager->GetNumRemainingJobs()
        : 0;
    OutPSORequests = PipelineStateCache::NumActivePrecacheRequests();

    return !RenderRegistrationFence.IsFenceComplete()
        || OutCompilingComponents > 0
        || OutShaderJobs > 0
        || OutPSORequests > 0;
}

void ULastFPSDestinationContentComponent::PollRenderWarmup()
{
    if (LoadState != EContentLoadState::WarmingRender)
    {
        return;
    }

    int32 CompilingComponents = 0;
    int32 ShaderJobs = 0;
    uint32 PSORequests = 0;
    const bool bBusy = IsRenderWarmupBusy(
        CompilingComponents,
        ShaderJobs,
        PSORequests);

    const bool bRenderFenceComplete = RenderRegistrationFence.IsFenceComplete();
    const int32 TotalComponents = WarmupComponents.Num();
    const float MeasuredRenderProgress = TotalComponents > 0
        ? 1.0f - static_cast<float>(CompilingComponents) / TotalComponents
        : 1.0f;
    const float CappedRenderProgress = bRenderFenceComplete
        ? MeasuredRenderProgress
        : FMath::Min(MeasuredRenderProgress, 0.95f);
    RenderStageProgress = FMath::Max(
        RenderStageProgress,
        FMath::Clamp(CappedRenderProgress, 0.0f, 0.95f));

    const int32 RemainingShaderWork =
        FMath::Max(ShaderJobs, 0) + static_cast<int32>(PSORequests);
    MaxObservedShaderWork = FMath::Max(
        MaxObservedShaderWork,
        RemainingShaderWork);
    const float MeasuredShaderProgress = MaxObservedShaderWork > 0
        ? 1.0f - static_cast<float>(RemainingShaderWork) / MaxObservedShaderWork
        : 0.0f;
    ShaderStageProgress = FMath::Max(
        ShaderStageProgress,
        FMath::Clamp(MeasuredShaderProgress, 0.0f, 0.95f));
    UpdateLoadingProgress();

    StableRenderFrames = bBusy ? 0 : StableRenderFrames + 1;

    const FLastFPSRenderWarmupSettings Settings =
        ActiveContentSet ? ActiveContentSet->RenderWarmup : FLastFPSRenderWarmupSettings();
    const double ElapsedSeconds = FPlatformTime::Seconds() - WarmupStartSeconds;
    const int32 RequiredStableFrames = FMath::Max(1, Settings.MinimumStableFrames);

    if (!bBusy && StableRenderFrames >= RequiredStableFrames)
    {
        UE_LOG(LogLastFPSDestinationContent, Log,
            TEXT("플레이어 렌더 준비 완료 — 컴포넌트 %d개, %.1fms"),
            WarmupComponents.Num(),
            ElapsedSeconds * 1000.0);
        CompleteRenderWarmup();
        return;
    }

    if (ElapsedSeconds >= FMath::Max(1.0f, Settings.TimeoutSeconds))
    {
        UE_LOG(LogLastFPSDestinationContent, Warning,
            TEXT("플레이어 렌더 준비 시간 초과 — %.1fs 후 진행합니다. "
                 "컴파일 컴포넌트=%d, 셰이더 작업=%d, PSO 요청=%u"),
            ElapsedSeconds,
            CompilingComponents,
            ShaderJobs,
            PSORequests);
        CompleteRenderWarmup();
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick(
            this, &ThisClass::PollRenderWarmup);
    }
    else
    {
        CompleteRenderWarmup();
    }
}

void ULastFPSDestinationContentComponent::CompleteRenderWarmup()
{
    RenderStageProgress = 1.0f;
    ShaderStageProgress = 1.0f;
    UpdateLoadingProgress();

    if (RenderWarmupCompletedDelegate.IsBound())
    {
        RenderWarmupCompletedDelegate.Execute();
        RenderWarmupCompletedDelegate.Unbind();
    }
    FinishLoad();
}

void ULastFPSDestinationContentComponent::FinishLoad()
{
    if (LoadState == EContentLoadState::Ready)
    {
        return;
    }

    AssetStageProgress = 1.0f;
    PoolStageProgress = 1.0f;
    RenderStageProgress = 1.0f;
    ShaderStageProgress = 1.0f;
    LoadState = EContentLoadState::Ready;
    CompleteLoadingProgress();
    const double ElapsedMs =
        (FPlatformTime::Seconds() - LoadStartSeconds) * 1000.0;

    UE_LOG(LogLastFPSDestinationContent, Log,
        TEXT("목적지 콘텐츠 로드 완료 — %d단계, 총 %d개, %.1fms 동안 로딩 화면 유지"),
        LoadPhase,
        RequestedPaths.Num(),
        ElapsedMs);

    LastFPSDestinationContentDebug::Print(FString::Printf(
        TEXT("[로딩] 완료 — %d단계 / %d개 / %.1fms"),
        LoadPhase,
        RequestedPaths.Num(),
        ElapsedMs));

    OnContentReady.Broadcast();
}

void ULastFPSDestinationContentComponent::RegisterLoadingProgress()
{
    UGameInstance* GameInstance =
        GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULastFPSLoadingProcessSubsystem* LoadingProcesses = GameInstance
        ? GameInstance->GetSubsystem<ULastFPSLoadingProcessSubsystem>()
        : nullptr;
    if (!LoadingProcesses)
    {
        return;
    }

    LoadingProcesses->EnsureLoadingTrackingActive();

    const FLastFPSDestinationLoadingProgressSettings Settings =
        LastFPSDestinationContentProgress::ResolveSettings(ActiveContentSet);
    LoadingProcessHandle =
        LoadingProcesses->RegisterLoadingProcessForTargetShare(
            LastFPSDestinationContentProgress::Content,
            Settings.OverallProgressShare,
            true);
    UpdateLoadingProgress();
}

void ULastFPSDestinationContentComponent::UpdateLoadingProgress()
{
    if (!LoadingProcessHandle.IsValid())
    {
        return;
    }

    UGameInstance* GameInstance =
        GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULastFPSLoadingProcessSubsystem* LoadingProcesses = GameInstance
        ? GameInstance->GetSubsystem<ULastFPSLoadingProcessSubsystem>()
        : nullptr;
    if (!LoadingProcesses)
    {
        return;
    }

    const FLastFPSDestinationLoadingProgressSettings Settings =
        LastFPSDestinationContentProgress::ResolveSettings(ActiveContentSet);
    const float TotalStageWeight =
        Settings.AssetAndGameplayCueWeight
        + Settings.ActorPoolWeight
        + Settings.RenderComponentWeight
        + Settings.ShaderAndPSOWeight;
    const float WeightedProgress =
        AssetStageProgress * Settings.AssetAndGameplayCueWeight
        + PoolStageProgress * Settings.ActorPoolWeight
        + RenderStageProgress * Settings.RenderComponentWeight
        + ShaderStageProgress * Settings.ShaderAndPSOWeight;
    const float ContentProgress = TotalStageWeight > KINDA_SMALL_NUMBER
        ? WeightedProgress / TotalStageWeight
        : 0.0f;
    LoadingProcesses->SetLoadingProcessProgress(
        LoadingProcessHandle,
        FMath::Clamp(ContentProgress, 0.0f, 1.0f));
}

void ULastFPSDestinationContentComponent::CompleteLoadingProgress()
{
    if (!LoadingProcessHandle.IsValid())
    {
        return;
    }

    UGameInstance* GameInstance =
        GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (ULastFPSLoadingProcessSubsystem* LoadingProcesses = GameInstance
        ? GameInstance->GetSubsystem<ULastFPSLoadingProcessSubsystem>()
        : nullptr)
    {
        LoadingProcesses->CompleteLoadingProcess(LoadingProcessHandle);
    }
}

void ULastFPSDestinationContentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    OnAssetsLoaded.Clear();
    OnContentReady.Clear();

    for (const TSharedPtr<FStreamableHandle>& LoadHandle : LoadHandles)
    {
        if (LoadHandle.IsValid())
        {
            LoadHandle->CancelHandle();
        }
    }
    LoadHandles.Reset();
    RequestedPaths.Reset();
    WarmupComponents.Reset();
    RenderWarmupCompletedDelegate.Unbind();
    ActiveContentSet = nullptr;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AssetProgressTimerHandle);
    }
    LoadingProcessHandle = FLastFPSLoadingProcessHandle();

    Super::EndPlay(EndPlayReason);
}
