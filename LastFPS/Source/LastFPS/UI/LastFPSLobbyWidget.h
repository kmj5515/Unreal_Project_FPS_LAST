#pragma once

#include "UI/LastFPSActivatableWidget.h"
#include "LastFPSLobbyWidget.generated.h"

class UButton;
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

	// ── 주요 액션 ─────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_StartMatch;

	// ── 퀵 메뉴 ──────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Inventory;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Missions;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Shop;

	// ── 하단 ──────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Settings;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_BackToMain;

private:
	UFUNCTION() void HandleStartMatchClicked();
	UFUNCTION() void HandleInventoryClicked();
	UFUNCTION() void HandleMissionsClicked();
	UFUNCTION() void HandleShopClicked();
	UFUNCTION() void HandleSettingsClicked();
	UFUNCTION() void HandleBackToMainClicked();

	void ShowWIPNotice(const FText& FeatureName);
};
