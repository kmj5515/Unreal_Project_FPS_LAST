#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSLoadingScreenWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;
class ULastFPSLoadingScreenSet;

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

    /** 로딩마다 랜덤 선택된 이미지+팁이 적용될 때 호출 (WBP에서 연출/애니메이션용) */
    UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Loading")
    void OnLoadingTipSelected(UTexture2D* Image, const FText& TipTitle, const FText& TipBody);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void RefreshFromGameInstance();

    /** LoadingSet에서 1세트를 랜덤 선택해 이미지/팁 위젯에 적용 */
    void ApplyRandomTip();

    /** WBP_Loading에서 동일 이름으로 만들면 자동 바인딩 */
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Status;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_MapName;

    /** 없으면 C++에서 막대 애니메이션을 생략 */
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_Loading;

    /** 랜덤 선택된 이미지가 들어갈 슬롯 (WBP에 동일 이름 UImage 만들면 자동 바인딩) */
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UImage> Img_Tip;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_TipTitle;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_TipBody;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Loading", meta=(ClampMin="0.05"))
    float IndeterminateCycleSeconds = 1.5f;

    /** 로딩마다 랜덤으로 보여줄 이미지+팁 세트 (WBP 클래스 디폴트에서 지정) */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Loading")
    TObjectPtr<ULastFPSLoadingScreenSet> LoadingSet;

private:
    void HandleTravelPresentationChanged(const FText& StatusText, const FText& MapNameText);

    FDelegateHandle TravelPresentationChangedHandle;
    float IndeterminatePhase = 0.0f;
};
