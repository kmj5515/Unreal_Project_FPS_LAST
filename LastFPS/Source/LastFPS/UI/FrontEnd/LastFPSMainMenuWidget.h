#pragma once

#include "UI/Framework/LastFPSActivatableWidget.h"
#include "LastFPSMainMenuWidget.generated.h"

class ULastFPSButtonBase;

UCLASS()
class LASTFPS_API ULastFPSMainMenuWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Start;

	/** 마스터 로비 서버 접속. 주소는 LastFPS Master Lobby 설정에서 온다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_MasterLobby;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Settings;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Quit;

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleMasterLobbyClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleQuitConfirmResult(bool bConfirmed);
};
