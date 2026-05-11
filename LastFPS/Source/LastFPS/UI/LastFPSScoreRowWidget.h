#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "LastFPSScoreRowWidget.generated.h"

/** 스코어보드 한 행. 헤더와 데이터 행 모두 동일 위젯을 사용해 컬럼 폭이 자동 정렬됨. */
UCLASS()
class LASTFPS_API ULastFPSScoreRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 데이터 행 채우기 (서버/클라 PlayerState 통계 그대로 표시) */
    void SetRow(const FString& PlayerName, int32 Kills, int32 Deaths, int32 Assists, int32 Damage, int32 DamageTaken, int32 HealingReceived, bool bIsLocalPlayer);

    /** 헤더 행 채우기 — 데이터 행과 같은 위젯/컬럼 폭으로 자동 정렬 */
    void SetHeader();

    /** 본인 행 강조 / 헤더 강조 등 BP에서 폴리싱 가능 */
    UFUNCTION(BlueprintImplementableEvent, Category="Scoreboard|Row")
    void OnRowStyleChanged(bool bIsHeader, bool bIsLocalPlayer);

protected:
    UPROPERTY(BlueprintReadOnly, Category="Scoreboard|Row", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> PlayerNameText;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard|Row", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> KillsText;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard|Row", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> DeathsText;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard|Row", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> AssistsText;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard|Row", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> DamageText;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard|Row", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> DamageTakenText;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard|Row", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> HealingReceivedText;
};
