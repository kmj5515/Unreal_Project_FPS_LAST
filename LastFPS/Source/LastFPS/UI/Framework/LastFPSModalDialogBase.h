#pragma once

#include "Messaging/CommonGameDialog.h"
#include "LastFPSModalDialogBase.generated.h"

class UTextBlock;
class USoundBase;

UCLASS(Abstract)
class LASTFPS_API ULastFPSModalDialogBase : public UCommonGameDialog
{
	GENERATED_BODY()

public:
	ULastFPSModalDialogBase();

	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void SetDialogText(const FText& InTitle, const FText& InBody);

	virtual void SetupDialog(
		UCommonGameDialogDescriptor* Descriptor,
		FCommonMessagingResultDelegate ResultCallback) override;

	virtual void KillDialog() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

	void CompleteDialog(ECommonMessagingResult Result);

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Body;

	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
	bool bCloseOnEscape = true;

	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI|Sound")
	TObjectPtr<USoundBase> ActivateSound;

	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI|Sound")
	TObjectPtr<USoundBase> DeactivateSound;

private:
	FCommonMessagingResultDelegate PendingResultCallback;
	bool bResultCompleted = false;
};
