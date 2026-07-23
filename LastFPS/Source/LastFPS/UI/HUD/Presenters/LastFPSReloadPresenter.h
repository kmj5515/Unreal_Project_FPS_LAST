#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LastFPSReloadPresenter.generated.h"

class UImage;
class UOverlay;

/**
 * 리로드 진행 표시를 HUD View에서 분리한다.
 * 진행률(0~1)을 이미지의 머티리얼 파라미터로 채운다. 소요 시간은 시작 알림 값을 기준으로
 * 자체 누적하며, 실제 표시 종료는 어빌리티의 완료/취소 알림에서 처리한다.
 * 무기 컴포넌트의 리로드 델리게이트가 이 Presenter의 UFUNCTION에 직접 바인딩된다.
 */
UCLASS()
class LASTFPS_API ULastFPSReloadPresenter final : public UObject
{
    GENERATED_BODY()

public:
    /** 진행을 표시할 이미지(null 허용)와 머티리얼 파라미터 이름을 받아 구성한다. */
    void Initialize(UImage* InReloadImage, UOverlay* Overlay, FName InProgressParameterName);

    /** 리로드 표시를 숨긴다. 최초 바인딩 시 초기 상태 정리에 사용한다. */
    void SetVisible(bool bVisible);

    /** 진행률을 매 HUD 갱신 주기마다 누적·반영한다. */
    void Tick(float DeltaTime);
    
    void SetReloadStarted(float ReloadDuration);
    void SetReloadFinished(bool bCompleted);

private:
    void UpdateProgress(float Progress);

    UPROPERTY(Transient)
    TObjectPtr<UImage> ReloadImage;
    
    UPROPERTY(Transient)
    TObjectPtr<UOverlay> ReloadOverlay;

    FName ProgressParameterName = TEXT("ReloadProgress");

    bool bReloadInProgress = false;
    float ReloadElapsedSeconds = 0.f;
    float ReloadTotalSeconds = 0.f;
};
