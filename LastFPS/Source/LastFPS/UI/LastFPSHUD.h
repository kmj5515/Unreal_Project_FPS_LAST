#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LastFPSHUD.generated.h"

class ULastFPSHUDWidget;
class ULastFPSScoreboardWidget;
class ALastFPSMatchGameState;

UCLASS()
class LASTFPS_API ALastFPSHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void ShowHitMarker();
    void ShowScoreboard();
    void HideScoreboard();

protected:
    UPROPERTY(EditDefaultsOnly, Category="HUD")
    TSubclassOf<ULastFPSHUDWidget> HUDWidgetClass;

    // BP_HUD에서 WBP_Scoreboard 클래스를 할당
    UPROPERTY(EditDefaultsOnly, Category="HUD")
    TSubclassOf<ULastFPSScoreboardWidget> ScoreboardWidgetClass;

private:
    UPROPERTY()
    TObjectPtr<ULastFPSHUDWidget> HUDWidget;

    UPROPERTY()
    TObjectPtr<ULastFPSScoreboardWidget> ScoreboardWidget;

    /** GameState 복제가 늦을 수 있어 BeginPlay에서 한 번 시도 후 실패 시 재시도 */
    void TryBindMatchGameState();

    UFUNCTION()
    void RetryBindMatchGameState();

    /** 매치 종료 시 호출: 스코어보드 표시 + 입력 매핑 클리어 */
    void HandleMatchEnded();

    FTimerHandle BindRetryTimerHandle;
    TWeakObjectPtr<ALastFPSMatchGameState> BoundMatchGameState;
    FDelegateHandle MatchEndedHandle;
};
