#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "LastFPSScoreboardWidget.generated.h"

class ULastFPSScoreRowWidget;

UCLASS()
class LASTFPS_API ULastFPSScoreboardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** GameState/PlayerArray에서 최신 데이터를 읽어 헤더 1행 + 플레이어 N행으로 채움 */
    UFUNCTION(BlueprintCallable, Category="Scoreboard")
    void RefreshScoreboard();

    /** 표시 중 자동 갱신 시작/중단 — HUD가 Show/Hide 시 호출 */
    UFUNCTION(BlueprintCallable, Category="Scoreboard")
    void StartAutoRefresh();

    UFUNCTION(BlueprintCallable, Category="Scoreboard")
    void StopAutoRefresh();

protected:
    virtual void NativeDestruct() override;

    /** 표시 중 자동 갱신 주기(초). 너무 짧게 두면 N×플레이어 만큼 위젯 재생성 비용 발생 */
    UPROPERTY(EditDefaultsOnly, Category="Scoreboard", meta=(ClampMin="0.5"))
    float AutoRefreshInterval = 5.0f;

    /** 매치 결과 헤더 — 우승자 / 종료 사유 (매치 종료 전에는 빈 문자열) */
    UPROPERTY(BlueprintReadOnly, Category="Scoreboard", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> MatchResultText;

    /** 헤더 행 + 플레이어 행을 자식으로 추가할 컨테이너 (Vertical Box 권장) */
    UPROPERTY(BlueprintReadOnly, Category="Scoreboard", meta=(BindWidgetOptional))
    TObjectPtr<UPanelWidget> RowsContainer;

    /** 헤더와 데이터 행 모두 동일한 클래스(WBP_ScoreRow)로 생성 → 컬럼 폭 자동 정렬 */
    UPROPERTY(EditDefaultsOnly, Category="Scoreboard")
    TSubclassOf<ULastFPSScoreRowWidget> RowWidgetClass;

private:
    FTimerHandle AutoRefreshTimerHandle;
};
