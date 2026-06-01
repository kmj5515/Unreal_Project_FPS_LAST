#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSLoadingScreenWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class LASTFPS_API ULastFPSLoadingScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="LastFPS|Loading")
    void SetStatusText(const FText& InText);

    UFUNCTION(BlueprintCallable, Category="LastFPS|Loading")
    void SetMapNameText(const FText& InText);

    UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Loading")
    void OnLoadingScreenUpdated(const FText& StatusText, const FText& MapNameText);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void RefreshFromGameInstance();

    /** WBP_Loading에서 동일 이름으로 만들면 자동 바인딩 */
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Status;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_MapName;

    /** 없으면 C++에서 막대 애니메이션을 생략 */
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Loading;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Loading", meta=(ClampMin="0.05"))
    float IndeterminateCycleSeconds = 1.5f;

private:
    float IndeterminatePhase = 0.0f;
};
