#pragma once

#include "UI/Framework/LastFPSActivatableWidget.h"
#include "LastFPSGameMenuWidget.generated.h"

class ULastFPSButtonBase;
class UWidget;

/**
 * 인게임 ESC 메뉴 전용 화면.
 *
 * 허브의 콘텐츠 탭과 수명이 전혀 다르므로 허브 셸을 재사용하지 않는다. 이 화면은 현재 게임 장면 위에
 * 잠시 올라오는 메뉴만 담당하고, 실제 인벤토리와 옵션 화면은 기존 화면 레지스트리를 통해 연다.
 */
UCLASS()
class LASTFPS_API ULastFPSGameMenuWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Inventory;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Settings;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Quit;

private:
	void HandleInventoryClicked();
	void HandleSettingsClicked();
	void HandleQuitClicked();
};
