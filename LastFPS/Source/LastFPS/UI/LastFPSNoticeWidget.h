#pragma once

#include "UI/LastFPSModalDialogBase.h"
#include "LastFPSNoticeWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLastFPSNoticeClosed);

UCLASS()
class LASTFPS_API ULastFPSNoticeWidget : public ULastFPSModalDialogBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void SetupNotice(const FText& InTitle, const FText& InBody);

	UPROPERTY(BlueprintAssignable, Category="LastFPS|UI")
	FOnLastFPSNoticeClosed OnNoticeClosed;

protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Ok;

	UFUNCTION()
	void HandleOkClicked();

	void CloseNotice();
};
