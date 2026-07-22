#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LoadingProcessInterface.h"
#include "LastFPSDestinationContentComponent.generated.h"

class ULastFPSDestinationContentSet;
struct FStreamableHandle;

/** 로드 완료(또는 로드할 것 없음) 시 1회. 스폰을 미뤄 둔 GameMode 가 구독한다. */
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

    /** GameMode 가 InitGameState 에서 호출한다. 목록이 비면 즉시 Ready 로 끝난다. */
    void StartContentLoad(const ULastFPSDestinationContentSet* ContentSet);

    bool IsContentReady() const { return LoadState == EContentLoadState::Ready; }

    FOnLastFPSDestinationContentReady OnContentReady;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    enum class EContentLoadState : uint8
    {
        Unloaded,
        Loading,
        Ready,
    };

    void HandleLoadCompleted();
    void FinishLoad();

    // 콘텐츠를 상주시키는 주체. 로컬 변수로 두면 로드 직후 해제된다.
    TSharedPtr<FStreamableHandle> LoadHandle;

    EContentLoadState LoadState = EContentLoadState::Unloaded;
    double LoadStartSeconds = 0.0;
};
