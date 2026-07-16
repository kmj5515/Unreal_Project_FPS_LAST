#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Utility/LastFPSTravelTypes.h"
#include "LastFPSLoadingProcessSubsystem.generated.h"

class APlayerController;
class ULoadingProcessTask;

USTRUCT(BlueprintType)
struct FLastFPSLoadingProcessHandle
{
    GENERATED_BODY()

    bool IsValid() const { return Id.IsValid(); }

private:
    friend class ULastFPSLoadingProcessSubsystem;

    UPROPERTY()
    FGuid Id;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(
    FOnLastFPSLoadingProgressChanged,
    float,
    FGameplayTag);

DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnLastFPSLoadingFinished,
    bool);

/**
 * 맵 전환 중 필요한 로딩 프로세스를 등록하고 전체 진행률과 화면 수명을 관리한다.
 * 개별 시스템은 구체적인 로딩 화면을 알지 않고 프로세스 핸들만 완료한다.
 */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSLoadingProcessSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    void BeginTravelLoading(ELastFPSTravelDestination Destination);
    void CancelLoading(const FString& FailureReason);

    FLastFPSLoadingProcessHandle RegisterLoadingProcess(
        FGameplayTag ProcessTag,
        float Weight,
        bool bRequired = true);

    bool SetLoadingProcessProgress(const FLastFPSLoadingProcessHandle& Handle, float Progress);
    bool CompleteLoadingProcess(const FLastFPSLoadingProcessHandle& Handle);

    void NotifyLevelLoadComplete(UWorld* LoadedWorld);
    void NotifyLocalPlayerControllerReady(APlayerController* ReadyPlayerController);
    void NotifyLocalPlayerPawnReady(APlayerController* ReadyPlayerController);

    bool IsLoadingActive() const { return bLoadingActive; }
    bool IsWaitingForLocalPlayerPawn() const;
    float GetOverallProgress() const { return DisplayedProgress; }

    FOnLastFPSLoadingProgressChanged OnLoadingProgressChanged;
    FOnLastFPSLoadingFinished OnLoadingFinished;

private:
    struct FLoadingProcessEntry
    {
        FGameplayTag Tag;
        float Weight = 1.0f;
        float Progress = 0.0f;
        bool bRequired = true;
    };

    void RegisterBuiltInProcesses(ELastFPSTravelDestination Destination);
    void BroadcastProgress(FGameplayTag ChangedProcessTag);
    void TryCompleteLoading();
    void CompleteLoading();
    void ReleaseLoadingScreenTask();
    void ResetSessionState();
    bool AreRequiredProcessesComplete() const;
    float CalculateOverallProgress() const;
    static bool DoesDestinationRequirePlayerPawn(ELastFPSTravelDestination Destination);

    UPROPERTY(Config, EditAnywhere, Category="LastFPS|Loading", meta=(ClampMin="0.0", ForceUnits="s"))
    float MinimumLoadingScreenDisplaySeconds = 1.0f;

    UPROPERTY(Transient)
    TObjectPtr<ULoadingProcessTask> LoadingScreenTask;

    TMap<FGuid, FLoadingProcessEntry> Processes;
    FLastFPSLoadingProcessHandle LevelProcessHandle;
    FLastFPSLoadingProcessHandle ControllerProcessHandle;
    FLastFPSLoadingProcessHandle PawnProcessHandle;

    ELastFPSTravelDestination ActiveDestination = ELastFPSTravelDestination::MainMenu;
    FTimerHandle CompletionTimerHandle;
    double LoadingStartSeconds = 0.0;
    float DisplayedProgress = 0.0f;
    bool bLoadingActive = false;
};
