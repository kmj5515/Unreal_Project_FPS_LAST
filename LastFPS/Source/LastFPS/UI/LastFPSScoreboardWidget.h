#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSScoreboardWidget.generated.h"

USTRUCT(BlueprintType)
struct FPlayerScoreRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
    FString PlayerName;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
    int32 Kills = 0;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
    int32 Deaths = 0;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
    int32 Assists = 0;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
    int32 DamageDealt = 0;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
    int32 DamageTaken = 0;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
    int32 HealingReceived = 0;

    UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
    bool bIsLocalPlayer = false;
};

UCLASS()
class LASTFPS_API ULastFPSScoreboardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 호출 시 PlayerArray에서 최신 데이터를 읽어 OnScoreboardRefreshed를 발동 */
    UFUNCTION(BlueprintCallable, Category="Scoreboard")
    void RefreshScoreboard();

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Scoreboard")
    void OnScoreboardRefreshed(const TArray<FPlayerScoreRow>& Rows);
};
