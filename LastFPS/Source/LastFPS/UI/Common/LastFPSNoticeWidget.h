#pragma once

#include "UI/Framework/LastFPSModalDialogBase.h"
#include "LastFPSNoticeWidget.generated.h"

class ULastFPSButtonBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLastFPSNoticeClosed);

UCLASS()
class LASTFPS_API ULastFPSNoticeWidget : public ULastFPSModalDialogBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void SetupNotice(const FText& InTitle, const FText& InBody);

	virtual void SetupDialog(
		UCommonGameDialogDescriptor* Descriptor,
		FCommonMessagingResultDelegate ResultCallback) override;

	virtual void KillDialog() override;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|UI")
	FOnLastFPSNoticeClosed OnNoticeClosed;

protected:
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Ok;

	UFUNCTION()
	void HandleOkClicked();

	void CloseNotice();

private:
	ECommonMessagingResult CloseResult = ECommonMessagingResult::Confirmed;
};
