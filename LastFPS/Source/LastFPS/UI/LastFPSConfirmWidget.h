#pragma once

#include "UI/LastFPSModalDialogBase.h"
#include "LastFPSConfirmWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSConfirmResult, bool, bConfirmed);

UCLASS()
class LASTFPS_API ULastFPSConfirmWidget : public ULastFPSModalDialogBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void SetupConfirm(const FText& InTitle, const FText& InBody);

	UPROPERTY(BlueprintAssignable, Category="LastFPS|UI")
	FOnLastFPSConfirmResult OnConfirmResult;

protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Confirm;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Cancel;

	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	void CloseWithResult(bool bConfirmed);
};
