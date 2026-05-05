#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LastFPSHUD.generated.h"

class ULastFPSHUDWidget;
class ULastFPSScoreboardWidget;

UCLASS()
class LASTFPS_API ALastFPSHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

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
};
