#pragma once

#include "Engine/TimerHandle.h"
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

	/** ESC 로 닫기. ULastFPSActivatableWidget 과 파생이 달라 같은 처리를 여기에도 둔다. */
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

	void CompleteDialog(ECommonMessagingResult Result);
	void DeactivateWithAnimation();
	void CancelOutAnimationFallback();
	void HandleOutAnimationFallbackElapsed();

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

	/** 종료 애니메이션 완료 이벤트가 누락될 때 모달을 닫기 위해 실제 길이에 더하는 안전 여유 시간이다. */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI|Animation", meta=(ClampMin="0.0", UIMin="0.0"))
	float OutAnimationFallbackPaddingSeconds = 0.25f;

	/** ESC 로 이 모달을 닫을지. */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
	bool bCloseOnEscape = true;

	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI|Sound")
	TObjectPtr<USoundBase> ActivateSound;

	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI|Sound")
	TObjectPtr<USoundBase> DeactivateSound;

private:
	FCommonMessagingResultDelegate PendingResultCallback;
	FTimerHandle OutAnimationFallbackTimerHandle;
	bool bResultCompleted = false;
	bool bWaitingForOutAnimation = false;
};
