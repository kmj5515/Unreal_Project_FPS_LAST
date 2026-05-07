#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSLobbyWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class LASTFPS_API ULastFPSLobbyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // WBP_Lobby에서 아래 이름 그대로 만들면 자동 바인딩됨
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UButton> Button_Ready;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Status;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_TimeRemaining;

private:
    UFUNCTION()
    void HandleReadyClicked();

    void UpdateStatusText(const FString& InText);

    bool bIsReady = false;
};

