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
