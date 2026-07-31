#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "LastFPSLoadingIndicatorWidget.generated.h"

class STextBlock;

/**
 * 전체 로딩 화면이 준비되기 전 짧은 비동기 구간을 표시하는 경량 인디케이터다.
 * 맵 로딩 화면의 책임과 분리하여 기존 화면 위에 작은 진행 표시만 제공한다.
 */
UCLASS()
class LASTFPS_API ULastFPSLoadingIndicatorWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	ULastFPSLoadingIndicatorWidget(const FObjectInitializer& ObjectInitializer);

	void SetStatusText(const FText& InStatusText);
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	UPROPERTY(BlueprintReadOnly, Category="LastFPS|Loading Indicator")
	FText StatusText;
};
