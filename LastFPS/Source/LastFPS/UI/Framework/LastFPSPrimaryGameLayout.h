#pragma once

#include "PrimaryGameLayout.h"
#include "LastFPSPrimaryGameLayout.generated.h"

class UCommonActivatableWidgetContainerBase;
class UCanvasPanel;

/**
 * Root UI layout with four CommonUI layers (Game / GameMenu / Menu / Modal).
 * Layers are created in C++ so a WBP child is optional; override in WBP to customize layout.
 */
UCLASS(Blueprintable)
class LASTFPS_API ULastFPSPrimaryGameLayout : public UPrimaryGameLayout
{
	GENERATED_BODY()

public:
	ULastFPSPrimaryGameLayout(const FObjectInitializer& ObjectInitializer);

	/**
	 * 가장 위 레이어(Modal→Menu→GameMenu)부터 최초로 발견되는 활성 위젯에 키보드 포커스를 준다.
	 * 모달이 닫힌 뒤 그 아래 화면(다른 레이어라 재활성화되지 않음)으로 포커스를 되돌려
	 * ESC가 위젯 대신 PlayerController로 새는 것을 막는다. 열린 메뉴가 없으면(HUD만 남으면)
	 * 아무 것도 포커스하지 않아 ESC가 정상적으로 PC(ESC 메뉴 토글)로 흐른다.
	 */
	void RestoreFocusToTopActiveWidget();

protected:
	virtual void NativeOnInitialized() override;

	/** Optional WBP overrides — if set, RegisterLayer uses these instead of auto-created stacks */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetContainerBase> LayerStack_Game;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetContainerBase> LayerStack_GameMenu;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetContainerBase> LayerStack_Menu;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetContainerBase> LayerStack_Modal;

private:
	void EnsureLayersRegistered();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetContainerBase> AutoLayer_Game;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetContainerBase> AutoLayer_GameMenu;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetContainerBase> AutoLayer_Menu;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetContainerBase> AutoLayer_Modal;
};
