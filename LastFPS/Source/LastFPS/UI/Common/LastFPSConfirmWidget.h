#pragma once

#include "UI/Framework/LastFPSModalDialogBase.h"
#include "LastFPSConfirmWidget.generated.h"

class ULastFPSButtonBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSConfirmResult, bool, bConfirmed);

/**
 * 예/아니오 확인 팝업 1회 표시에 필요한 값 묶음.
 *
 * FCommonMessagingResultDelegate 는 동적 델리게이트가 아니라 USTRUCT 로 만들 수 없어 C++ 전용이다.
 * 버튼 라벨은 CreateConfirmationYesNo 의 기본값을 쓰고, 바꿔야 하면 여기에 필드를 늘린다.
 */
struct FLastFPSConfirmParams
{
	FText Title;
	FText Body;

	/**
	 * 결과 통보. 확인=Confirmed, 취소=Declined, ESC=Cancelled, 강제 종료=Killed.
	 * "취소"와 "그냥 닫음"을 구분해야 하므로 bool 이 아니라 결과 열거형으로 받는다.
	 */
	FCommonMessagingResultDelegate OnResult;
};

UCLASS()
class LASTFPS_API ULastFPSConfirmWidget : public ULastFPSModalDialogBase
{
	GENERATED_BODY()

public:
	/**
	 * 확인 팝업을 띄운다. 클래스가 이미 로드돼 있어야 하므로, 카탈로그에서 미리 로드하지 않는
	 * 팝업이라면 ShowAsyncPopup 을 쓴다(동기 경로는 그 자리에서 로드하느라 프레임이 멈춘다).
	 */
	static ULastFPSConfirmWidget* ShowPopup(
		const UObject* WorldContext,
		const FLastFPSConfirmParams& Params);

	/**
	 * 위젯 클래스를 비동기로 로드해 띄운다. 로드가 끝난 뒤 팝업이 생기므로 위젯을 돌려줄 수 없다.
	 * 결과는 Params.OnResult 로 받는다. 열지 못하면 Unknown 으로 한 번 통보된다.
	 */
	static void ShowAsyncPopup(
		const UObject* WorldContext,
		const FLastFPSConfirmParams& Params);

	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void SetupConfirm(const FText& InTitle, const FText& InBody);

	/** 파라미터를 표시 상태로 옮긴다. 동기·비동기 진입점이 같은 절차를 쓰도록 분리했다. */
	void ApplyParams(const FLastFPSConfirmParams& Params);

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
