#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LastFPSLobbyHUD.generated.h"

class ULastFPSLobbyWidget;

UCLASS()
class LASTFPS_API ALastFPSLobbyHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="LobbyHUD")
    TSubclassOf<ULastFPSLobbyWidget> LobbyWidgetClass;

private:
    /** PrimaryGameLayout 준비 후 UI.Layer.Menu에 로비 위젯 push (없으면 재시도) */
    void TryPushLobbyWidget();

    UFUNCTION()
    void RetryPushLobbyWidget();

    UPROPERTY()
    TObjectPtr<ULastFPSLobbyWidget> LobbyWidget;

    FTimerHandle UIPushRetryTimerHandle;
    bool bLobbyWidgetPushed = false;
};
