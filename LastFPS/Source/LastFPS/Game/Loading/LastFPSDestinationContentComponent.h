#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/Loading/LastFPSLoadingProcessSubsystem.h"
#include "LoadingProcessInterface.h"
#include "RenderCommandFence.h"
#include "LastFPSDestinationContentComponent.generated.h"

class AActor;
class UPrimitiveComponent;
class ULastFPSDestinationContentSet;
struct FStreamableHandle;

/** 에셋 의존성 로드 완료 시 1회. GameMode가 실제 Pawn을 생성하고 렌더 준비를 시작한다. */
DECLARE_MULTICAST_DELEGATE(FOnLastFPSDestinationAssetsLoaded);

/** 에셋과 렌더 준비가 모두 완료될 때 1회. 최종 게임플레이 준비 신호다. */
DECLARE_MULTICAST_DELEGATE(FOnLastFPSDestinationContentReady);

/**
 * 목적지 콘텐츠가 준비될 때까지 로딩 화면을 붙잡는 게이트.
 *
 * ULoadingScreenManager 가 매 프레임 GameState 의 컴포넌트들을 순회하며
 * ILoadingProcessInterface 를 질의하므로 별도 등록이 필요 없다.
 *
 * 현재는 서버(및 Standalone/ListenServer 호스트)만 로드를 수행한다.
 * 원격 클라이언트는 게이트가 걸리지 않으며, 이는 도입 전과 동일한 동작이다.
 */
UCLASS()
class ULastFPSDestinationContentComponent : public UActorComponent, public ILoadingProcessInterface
{
    GENERATED_BODY()

public:
    ULastFPSDestinationContentComponent();

    virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;

    /** GameMode가 InitGameState에서 호출한다. 단계별 에셋 의존성을 비동기로 준비한다. */
    void StartContentLoad(ULastFPSDestinationContentSet* ContentSet);

    /** 원격 클라이언트가 복제된 Primary Asset ID로 같은 콘텐츠 준비를 시작한다. */
    void StartContentLoad(const FPrimaryAssetId& ContentSetId);

    /**
     * 에셋 준비 콜백에서 실제로 생성한 Actor를 전달한다.
     * 등록된 렌더 컴포넌트의 PSO와 렌더 스레드 반영을 기다린 뒤 최종 Ready로 전환한다.
     */
    void BeginRenderWarmup(
        const TArray<AActor*>& Actors,
        FSimpleDelegate OnWarmupCompleted = FSimpleDelegate());

    /** GameMode의 풀 준비 구간을 목적지 콘텐츠 진행률에 반영한다. */
    void BeginActorPoolPreparation();
    void CompleteActorPoolPreparation();

    bool IsContentReady() const { return LoadState == EContentLoadState::Ready; }

    FOnLastFPSDestinationAssetsLoaded OnAssetsLoaded;
    FOnLastFPSDestinationContentReady OnContentReady;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    enum class EContentLoadState : uint8
    {
        Unloaded,
        LoadingAssets,
        AwaitingRenderWarmup,
        WarmingRender,
        Ready,
    };

    void BeginNextLoadPhase();
    void HandleContentSetLoaded();
    void PollAssetLoadProgress();
    void HandleLoadPhaseCompleted();
    void HandleAssetsLoaded();
    void PollRenderWarmup();
    void CompleteRenderWarmup();
    bool IsRenderWarmupBusy(int32& OutCompilingComponents, int32& OutShaderJobs,
        uint32& OutPSORequests) const;
    void FinishLoad();
    void RegisterLoadingProgress();
    void UpdateLoadingProgress();
    void CompleteLoadingProgress();
    void CreateDataDrivenRenderWarmupActors(TArray<AActor*>& OutActors);
    void DestroyOwnedRenderWarmupActors();

    /** 단계가 바뀌어도 앞 단계의 에셋을 목적지를 떠날 때까지 상주시킨다. */
    TArray<TSharedPtr<FStreamableHandle>> LoadHandles;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSDestinationContentSet> ActiveContentSet;

    /** 실제 플레이어 컴포넌트의 수명과 준비 상태만 관찰하며 소유하지 않는다. */
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<UPrimitiveComponent>> WarmupComponents;

    /** 데이터 계약이 만든 일시적인 Actor이며 PSO 준비가 끝나면 중앙 로더가 제거한다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<AActor>> OwnedRenderWarmupActors;

    TSet<FSoftObjectPath> RequestedPaths;
    FPrimaryAssetId PendingContentSetId;

    EContentLoadState LoadState = EContentLoadState::Unloaded;
    double LoadStartSeconds = 0.0;
    double WarmupStartSeconds = 0.0;
    int32 LoadPhase = 0;
    int32 StableRenderFrames = 0;
    int32 MaxObservedShaderWork = 0;
    float AssetStageProgress = 0.0f;
    float PoolStageProgress = 0.0f;
    float RenderStageProgress = 0.0f;
    float ShaderStageProgress = 0.0f;
    FRenderCommandFence RenderRegistrationFence;
    FSimpleDelegate RenderWarmupCompletedDelegate;
    FLastFPSLoadingProcessHandle LoadingProcessHandle;
    FTimerHandle AssetProgressTimerHandle;
};
