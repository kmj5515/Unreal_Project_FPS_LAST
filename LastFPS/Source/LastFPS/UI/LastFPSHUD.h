#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LastFPSHUD.generated.h"

class ULastFPSHUDWidget;

UCLASS()
class LASTFPS_API ALastFPSHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

protected:
    // BP_HUD에서 WBP_HUD 클래스를 할당
    UPROPERTY(EditDefaultsOnly, Category="HUD")
    TSubclassOf<ULastFPSHUDWidget> HUDWidgetClass;

private:
    UPROPERTY()
    TObjectPtr<ULastFPSHUDWidget> HUDWidget;
};
