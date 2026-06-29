#pragma once

#include "UI/Framework/LastFPSActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "LastFPSLobbyWidget.generated.h"

class ULastFPSButtonBase;
class UTextBlock;

/**
 * 아웃게임 로비 화면 — Menu Layer
 * 버튼 뼈대만 구성. 기능은 각 서브 시스템 구현 후 연결.
 */
UCLASS()
class LASTFPS_API ULastFPSLobbyWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// ── 플레이어 정보 ─────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_PlayerName;

	// ── 퀵 메뉴 ──────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Inventory;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Missions;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Shop;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Module;

	// ── 하단 ──────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Settings;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_BackToMain;

private:
	UFUNCTION() void HandleInventoryClicked();
	UFUNCTION() void HandleMissionsClicked();
	UFUNCTION() void HandleShopClicked();
	UFUNCTION() void HandleModuleClicked();
	UFUNCTION() void HandleSettingsClicked();
	UFUNCTION() void HandleBackToMainClicked();

	/** 화면을 연다. 레지스트리에 없으면 "준비 중" 공지로 폴백. */
	void OpenScreenOrNotice(const FGameplayTag& ScreenTag, const FText& FeatureName);
};
