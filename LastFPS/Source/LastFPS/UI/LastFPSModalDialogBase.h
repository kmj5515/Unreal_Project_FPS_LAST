#pragma once

#include "UI/LastFPSActivatableWidget.h"
#include "LastFPSModalDialogBase.generated.h"

class UTextBlock;

UCLASS(Abstract)
class LASTFPS_API ULastFPSModalDialogBase : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void SetDialogText(const FText& InTitle, const FText& InBody);

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Body;
};
