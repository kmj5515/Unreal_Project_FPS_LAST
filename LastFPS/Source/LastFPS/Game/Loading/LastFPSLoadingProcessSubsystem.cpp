#include "Game/Loading/LastFPSLoadingProcessSubsystem.h"

#include "LoadingProcessTask.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSLoadingProcessSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSLoadingProcess, Log, All);

namespace LastFPSLoadingProcessTags
{
    UE_DEFINE_GAMEPLAY_TAG_STATIC(Level, "Loading.Process.Level");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(LocalPlayerController, "Loading.Process.LocalPlayer.Controller");
    UE_DEFINE_GAMEPLAY_TAG_STATIC(LocalPlayerPawn, "Loading.Process.LocalPlayer.Pawn");
}

void ULastFPSLoadingProcessSubsystem::Deinitialize()
{
    if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
    {
        World->GetTimerManager().ClearTimer(CompletionTimerHandle);
    }

    ReleaseLoadingScreenTask();
    ResetSessionState();
    Super::Deinitialize();
}

void ULastFPSLoadingProcessSubsystem::BeginTravelLoading(const ELastFPSTravelDestination Destination)
{
    if (bLoadingActive)
    {
        UE_LOG(LogLastFPSLoadingProcess, Warning,
            TEXT("진행 중인 로딩 세션을 새 맵 전환 요청으로 교체합니다."));
        ReleaseLoadingScreenTask();
    }

    if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
    {
        World->GetTimerManager().ClearTimer(CompletionTimerHandle);
    }

    ResetSessionState();
    bLoadingActive = true;
    ActiveDestination = Destination;
    LoadingStartSeconds = FPlatformTime::Seconds();

    RegisterBuiltInProcesses(Destination);

    LoadingScreenTask = ULoadingProcessTask::CreateLoadingScreenProcessTask(
        GetGameInstance(),
        FString::Printf(TEXT("LastFPS loading processes: %s"),
            *StaticEnum<ELastFPSTravelDestination>()->GetNameStringByValue(static_cast<int64>(Destination))));

    if (!LoadingScreenTask)
    {
        UE_LOG(LogLastFPSLoadingProcess, Warning,
            TEXT("CommonLoadingScreen 프로세스 작업을 생성하지 못했습니다."));
    }

    BroadcastProgress(FGameplayTag());
}

void ULastFPSLoadingProcessSubsystem::EnsureLoadingTrackingActive()
{
    if (bLoadingActive)
    {
        return;
    }

    ResetSessionState();
    bLoadingActive = true;
    LoadingStartSeconds = FPlatformTime::Seconds();

    UE_LOG(LogLastFPSLoadingProcess, Log,
        TEXT("외부 로딩 게이트의 진행률 추적 세션을 시작합니다."));
    BroadcastProgress(FGameplayTag());
}

void ULastFPSLoadingProcessSubsystem::CancelLoading(const FString& FailureReason)
{
    if (!bLoadingActive)
    {
        return;
    }

    if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
    {
        World->GetTimerManager().ClearTimer(CompletionTimerHandle);
    }

    bLoadingActive = false;
    ReleaseLoadingScreenTask();
    UE_LOG(LogLastFPSLoadingProcess, Error, TEXT("로딩 취소: %s"), *FailureReason);
    OnLoadingFinished.Broadcast(false);
}

FLastFPSLoadingProcessHandle ULastFPSLoadingProcessSubsystem::RegisterLoadingProcess(
    const FGameplayTag ProcessTag,
    const float Weight,
    const bool bRequired)
{
    FLastFPSLoadingProcessHandle Handle;
    if (!bLoadingActive || !ProcessTag.IsValid() || Weight <= 0.0f)
    {
        UE_LOG(LogLastFPSLoadingProcess, Warning,
            TEXT("로딩 프로세스 등록 실패: Active=%d, Tag=%s, Weight=%.3f"),
            bLoadingActive ? 1 : 0,
            *ProcessTag.ToString(),
            Weight);
        return Handle;
    }

    Handle.Id = FGuid::NewGuid();

    FLoadingProcessEntry& Entry = Processes.Add(Handle.Id);
    Entry.Tag = ProcessTag;
    Entry.Weight = Weight;
    Entry.bRequired = bRequired;

    BroadcastProgress(ProcessTag);
    return Handle;
}

FLastFPSLoadingProcessHandle
ULastFPSLoadingProcessSubsystem::RegisterLoadingProcessForTargetShare(
    const FGameplayTag ProcessTag,
    const float TargetShare,
    const bool bRequired)
{
    if (TargetShare <= 0.0f || TargetShare >= 1.0f)
    {
        UE_LOG(LogLastFPSLoadingProcess, Warning,
            TEXT("목표 점유율 기반 로딩 프로세스 등록 실패: Tag=%s, TargetShare=%.3f"),
            *ProcessTag.ToString(),
            TargetShare);
        return FLastFPSLoadingProcessHandle();
    }

    float ExistingTotalWeight = 0.0f;
    for (const TPair<FGuid, FLoadingProcessEntry>& Pair : Processes)
    {
        ExistingTotalWeight += FMath::Max(Pair.Value.Weight, 0.0f);
    }

    const float RelativeWeight = ExistingTotalWeight > KINDA_SMALL_NUMBER
        ? ExistingTotalWeight * TargetShare / (1.0f - TargetShare)
        : 1.0f;
    return RegisterLoadingProcess(
        ProcessTag,
        RelativeWeight,
        bRequired);
}

bool ULastFPSLoadingProcessSubsystem::SetLoadingProcessProgress(
    const FLastFPSLoadingProcessHandle& Handle,
    const float Progress)
{
    FLoadingProcessEntry* Entry = Processes.Find(Handle.Id);
    if (!bLoadingActive || !Entry)
    {
        return false;
    }

    Entry->Progress = FMath::Max(Entry->Progress, FMath::Clamp(Progress, 0.0f, 1.0f));
    BroadcastProgress(Entry->Tag);
    TryCompleteLoading();
    return true;
}

bool ULastFPSLoadingProcessSubsystem::CompleteLoadingProcess(const FLastFPSLoadingProcessHandle& Handle)
{
    return SetLoadingProcessProgress(Handle, 1.0f);
}

void ULastFPSLoadingProcessSubsystem::NotifyLevelLoadComplete(UWorld* LoadedWorld)
{
    if (!bLoadingActive || !LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
    {
        return;
    }

    UE_LOG(LogLastFPSLoadingProcess, Log, TEXT("레벨 로드 완료: %s"), *LoadedWorld->GetMapName());
    CompleteLoadingProcess(LevelProcessHandle);
}

void ULastFPSLoadingProcessSubsystem::NotifyLocalPlayerControllerReady(APlayerController* ReadyPlayerController)
{
    if (!bLoadingActive
        || !IsValid(ReadyPlayerController)
        || !ReadyPlayerController->IsLocalController()
        || ReadyPlayerController->GetGameInstance() != GetGameInstance())
    {
        return;
    }

    UE_LOG(LogLastFPSLoadingProcess, Log,
        TEXT("로컬 PlayerController 준비 완료: %s"), *GetNameSafe(ReadyPlayerController));
    CompleteLoadingProcess(ControllerProcessHandle);
}

void ULastFPSLoadingProcessSubsystem::NotifyLocalPlayerPawnReady(APlayerController* ReadyPlayerController)
{
    if (!IsWaitingForLocalPlayerPawn()
        || !IsValid(ReadyPlayerController)
        || !ReadyPlayerController->IsLocalController()
        || ReadyPlayerController->GetGameInstance() != GetGameInstance())
    {
        return;
    }

    APawn* ReadyPawn = ReadyPlayerController->GetPawn();
    if (!IsValid(ReadyPawn)
        || ReadyPawn->GetController() != ReadyPlayerController
        || !ReadyPawn->HasActorBegunPlay())
    {
        return;
    }

    UE_LOG(LogLastFPSLoadingProcess, Log,
        TEXT("로컬 Pawn 준비 완료: %s"), *GetNameSafe(ReadyPawn));
    CompleteLoadingProcess(PawnProcessHandle);
}

bool ULastFPSLoadingProcessSubsystem::IsWaitingForLocalPlayerPawn() const
{
    if (!bLoadingActive || !PawnProcessHandle.IsValid())
    {
        return false;
    }

    const FLoadingProcessEntry* Entry = Processes.Find(PawnProcessHandle.Id);
    return Entry && Entry->Progress < 1.0f;
}

void ULastFPSLoadingProcessSubsystem::RegisterBuiltInProcesses(const ELastFPSTravelDestination Destination)
{
    if (IsRunningDedicatedServer())
    {
        LevelProcessHandle = RegisterLoadingProcess(LastFPSLoadingProcessTags::Level, 1.0f);
        return;
    }

    if (DoesDestinationRequirePlayerPawn(Destination))
    {
        LevelProcessHandle = RegisterLoadingProcess(LastFPSLoadingProcessTags::Level, 0.65f);
        ControllerProcessHandle = RegisterLoadingProcess(LastFPSLoadingProcessTags::LocalPlayerController, 0.15f);
        PawnProcessHandle = RegisterLoadingProcess(LastFPSLoadingProcessTags::LocalPlayerPawn, 0.20f);
        return;
    }

    LevelProcessHandle = RegisterLoadingProcess(LastFPSLoadingProcessTags::Level, 0.80f);
    ControllerProcessHandle = RegisterLoadingProcess(LastFPSLoadingProcessTags::LocalPlayerController, 0.20f);
}

void ULastFPSLoadingProcessSubsystem::BroadcastProgress(const FGameplayTag ChangedProcessTag)
{
    // 동적 프로세스 추가로 분모가 늘더라도 사용자에게 보인 진행률은 뒤로 가지 않는다.
    DisplayedProgress = FMath::Max(DisplayedProgress, CalculateOverallProgress());
    OnLoadingProgressChanged.Broadcast(DisplayedProgress, ChangedProcessTag);
}

void ULastFPSLoadingProcessSubsystem::TryCompleteLoading()
{
    if (!bLoadingActive || !AreRequiredProcessesComplete())
    {
        return;
    }

    const double ElapsedSeconds = FPlatformTime::Seconds() - LoadingStartSeconds;
    const float RemainingSeconds = FMath::Max(
        MinimumLoadingScreenDisplaySeconds - static_cast<float>(ElapsedSeconds),
        0.0f);

    if (RemainingSeconds > KINDA_SMALL_NUMBER)
    {
        if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
            World && !World->GetTimerManager().IsTimerActive(CompletionTimerHandle))
        {
            World->GetTimerManager().SetTimer(
                CompletionTimerHandle,
                this,
                &ULastFPSLoadingProcessSubsystem::CompleteLoading,
                RemainingSeconds,
                false);
        }
        return;
    }

    CompleteLoading();
}

void ULastFPSLoadingProcessSubsystem::CompleteLoading()
{
    if (!bLoadingActive || !AreRequiredProcessesComplete())
    {
        return;
    }

    DisplayedProgress = 1.0f;
    OnLoadingProgressChanged.Broadcast(DisplayedProgress, FGameplayTag());
    bLoadingActive = false;
    ReleaseLoadingScreenTask();

    UE_LOG(LogLastFPSLoadingProcess, Log,
        TEXT("모든 필수 로딩 프로세스 완료: %d개"), Processes.Num());
    OnLoadingFinished.Broadcast(true);
}

void ULastFPSLoadingProcessSubsystem::ReleaseLoadingScreenTask()
{
    if (LoadingScreenTask)
    {
        LoadingScreenTask->Unregister();
        LoadingScreenTask = nullptr;
    }
}

void ULastFPSLoadingProcessSubsystem::ResetSessionState()
{
    Processes.Reset();
    LevelProcessHandle = FLastFPSLoadingProcessHandle();
    ControllerProcessHandle = FLastFPSLoadingProcessHandle();
    PawnProcessHandle = FLastFPSLoadingProcessHandle();
    DisplayedProgress = 0.0f;
    LoadingStartSeconds = 0.0;
    bLoadingActive = false;
}

bool ULastFPSLoadingProcessSubsystem::AreRequiredProcessesComplete() const
{
    if (Processes.IsEmpty())
    {
        return false;
    }

    for (const TPair<FGuid, FLoadingProcessEntry>& Pair : Processes)
    {
        if (Pair.Value.bRequired && Pair.Value.Progress < 1.0f)
        {
            return false;
        }
    }
    return true;
}

float ULastFPSLoadingProcessSubsystem::CalculateOverallProgress() const
{
    float TotalWeight = 0.0f;
    float WeightedProgress = 0.0f;

    for (const TPair<FGuid, FLoadingProcessEntry>& Pair : Processes)
    {
        TotalWeight += Pair.Value.Weight;
        WeightedProgress += Pair.Value.Weight * Pair.Value.Progress;
    }

    return TotalWeight > KINDA_SMALL_NUMBER
        ? FMath::Clamp(WeightedProgress / TotalWeight, 0.0f, 1.0f)
        : 0.0f;
}

bool ULastFPSLoadingProcessSubsystem::DoesDestinationRequirePlayerPawn(
    const ELastFPSTravelDestination Destination)
{
    return Destination == ELastFPSTravelDestination::Hub;
}
