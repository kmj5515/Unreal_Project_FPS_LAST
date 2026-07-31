#pragma once

#include "Messaging/CommonGameDialog.h"
#include "LastFPSModalDialogBase.generated.h"

class UTextBlock;
class USoundBase;
class UWidgetAnimation;

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
	virtual void OnAnimationFinished_Implementation(
		const UWidgetAnimation* Animation) override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

	void CompleteDialog(ECommonMessagingResult Result);
	void DeactivateWithAnimation();

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Body;

	/** 파생 WBP에 같은 이름의 애니메이션이 있을 때 모달 활성화 시 한 번 재생한다. */
	UPROPERTY(Transient, meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> InAnimation;

	/** 파생 WBP에 같은 이름의 애니메이션이 있을 때 재생을 마친 뒤 모달을 비활성화한다. */
	UPROPERTY(Transient, meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> OutAnimation;

	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
	bool bCloseOnEscape = true;

	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI|Sound")
	TObjectPtr<USoundBase> ActivateSound;

	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI|Sound")
	TObjectPtr<USoundBase> DeactivateSound;

private:
	FCommonMessagingResultDelegate PendingResultCallback;
	bool bResultCompleted = false;
	bool bWaitingForOutAnimation = false;
};
