#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LastFPSLoadingIndicatorSubsystem.generated.h"

class APlayerController;
class ULastFPSLoadingIndicatorWidget;

/**
 * 전체 로딩 화면으로 전환되기 전 표시할 인디케이터의 수명을 관리한다.
 * CommonLoadingScreen이 표시되는 즉시 인디케이터를 내려 두 UI가 겹치지 않도록 한다.
 */
UCLASS(Config=Game, DefaultConfig)
class LASTFPS_API ULastFPSLoadingIndicatorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	static void Show(
		UObject* WorldContextObject,
		APlayerController* OwningPlayer,
		const FText& StatusText);

	static void Hide(UObject* WorldContextObject);

	bool IsIndicatorVisible() const;

private:
	void ShowInternal(APlayerController* OwningPlayer, const FText& StatusText);
	void HideInternal();

	UPROPERTY(Config, EditAnywhere, Category="LastFPS|Loading Indicator")
	TSubclassOf<ULastFPSLoadingIndicatorWidget> IndicatorWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSLoadingIndicatorWidget> IndicatorWidget;
};
