#pragma once

#include "UI/Framework/LastFPSModalDialogBase.h"
#include "LastFPSConfirmWidget.generated.h"

class ULastFPSButtonBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSConfirmResult, bool, bConfirmed);

UCLASS()
class LASTFPS_API ULastFPSConfirmWidget : public ULastFPSModalDialogBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void SetupConfirm(const FText& InTitle, const FText& InBody);

	virtual void SetupDialog(
		UCommonGameDialogDescriptor* Descriptor,
		FCommonMessagingResultDelegate ResultCallback) override;

	virtual void KillDialog() override;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|UI")
	FOnLastFPSConfirmResult OnConfirmResult;

protected:
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Confirm;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Cancel;

	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	void CloseWithResult(bool bConfirmed);
	void CloseWithMessagingResult(
		bool bConfirmed,
		ECommonMessagingResult Result);

private:
	ECommonMessagingResult ConfirmResult = ECommonMessagingResult::Confirmed;
	ECommonMessagingResult CancelResult = ECommonMessagingResult::Declined;
	ECommonMessagingResult BackResult = ECommonMessagingResult::Cancelled;
};
