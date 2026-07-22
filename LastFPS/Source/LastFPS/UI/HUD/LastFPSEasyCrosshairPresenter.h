#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LastFPSEasyCrosshairPresenter.generated.h"

class UecsCrosshairEditorAsset;
class UecsCrosshairWidget;
class UOverlay;
class UWorld;

/** EasyCrosshair 플러그인의 수명과 애니메이션 호출을 HUD로부터 분리한다. */
UCLASS()
class LASTFPS_API ULastFPSEasyCrosshairPresenter final : public UObject
{
    GENERATED_BODY()

public:
    bool ShowCrosshair(
        UWorld& World,
        UecsCrosshairEditorAsset& CrosshairAsset,
        UOverlay* Host,
        FName FireAnimationName,
        float FireAnimationDuration);

    void SetVisible(bool bVisible);
    void PlayFireAnimation();
    void Shutdown();

private:
    TWeakObjectPtr<UWorld> OwningWorld;
    TWeakObjectPtr<UecsCrosshairWidget> CrosshairWidget;

    /** 원본 에셋 식별용 — 같은 무기 에셋 재적용 시 위젯 재생성을 피한다. */
    TWeakObjectPtr<const UecsCrosshairEditorAsset> SourceCrosshairAsset;

    /**
     * 원본 에셋의 런타임 임시 사본.
     * ecs는 에셋에 인스턴스된 Behavior에 런타임 위젯 포인터를 직접 써넣으므로,
     * 원본을 그대로 쓰면 공유 에셋이 PIE 위젯을 참조한 채 남아 에디터 Undo 버퍼를
     * 통해 GC 누수를 일으킨다(ReferenceChainSearch ensure). 사본은 Transient 패키지
     * 소속이라 위젯과 함께 통째로 GC된다.
     */
    UPROPERTY(Transient)
    TObjectPtr<UecsCrosshairEditorAsset> RuntimeCrosshairAsset;

    FName FireAnimationName = NAME_None;
    float FireAnimationDuration = 0.f;
    bool bMissingAnimationWarningLogged = false;
};
