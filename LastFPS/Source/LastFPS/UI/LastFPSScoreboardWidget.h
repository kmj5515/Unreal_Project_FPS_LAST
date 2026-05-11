#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "LastFPSScoreboardWidget.generated.h"

UCLASS()
class LASTFPS_API ULastFPSScoreboardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 호출 시 GameState/PlayerArray에서 최신 데이터를 읽어 텍스트 위젯들을 갱신 */
    UFUNCTION(BlueprintCallable, Category="Scoreboard")
    void RefreshScoreboard();

protected:
    /** 매치 결과 헤더 — 우승자 / 종료 사유 (매치 종료 전에는 빈 문자열) */
    UPROPERTY(BlueprintReadOnly, Category="Scoreboard", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> MatchResultText;

    /** 행별 통계 — 한 줄당 한 명, Monospace 폰트 권장 */
    UPROPERTY(BlueprintReadOnly, Category="Scoreboard", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> ScoreboardText;
};
