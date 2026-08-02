#pragma once

#include "UI/Framework/LastFPSModalDialogBase.h"
#include "LastFPSNoticeWidget.generated.h"

class ULastFPSButtonBase;

struct FLastFPSNoticeParams
{
	FText Title;
	FText Body;

	/** 닫힘 결과 통보. 비워 두면 결과를 받지 않는다. */
	FCommonMessagingResultDelegate OnResult;
};

UCLASS()
class LASTFPS_API ULastFPSNoticeWidget : public ULastFPSModalDialogBase
{
	GENERATED_BODY()

public:
	/**
	 * 공지 팝업을 띄운다. 태그와 설정 절차를 이 클래스가 소유하므로 호출부는 값만 채우면 된다.
	 *
	 * @param WorldContext 어느 로컬 플레이어의 팝업인지 정하는 데 필요하다(LocalPlayer 서브시스템).
	 * @return 열린 팝업. 카탈로그 미등록·레이아웃 미준비면 nullptr
	 */
	static ULastFPSNoticeWidget* ShowPopup(
		const UObject* WorldContext,
		const FLastFPSNoticeParams& Params);
	
	void SetupNotice(const FText& InTitle, const FText& InBody, FCommonMessagingResultDelegate ResultCallback);

	virtual void SetupDialog(
		UCommonGameDialogDescriptor* Descriptor,
		FCommonMessagingResultDelegate ResultCallback) override;

	virtual void KillDialog() override;
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
