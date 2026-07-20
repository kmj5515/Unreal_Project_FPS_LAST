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
    TWeakObjectPtr<UecsCrosshairEditorAsset> ActiveCrosshairAsset;
    FName FireAnimationName = NAME_None;
    float FireAnimationDuration = 0.f;
    bool bMissingAnimationWarningLogged = false;
};
