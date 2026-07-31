#pragma once

#include "PrimaryGameLayout.h"
#include "LastFPSPrimaryGameLayout.generated.h"

class UCommonActivatableWidgetContainerBase;
class UCanvasPanel;

/**
 * 프로젝트 공통 UI 레이어(Game / GameMenu / Menu / Modal / Overlay)를 소유하는 루트 레이아웃이다.
 * C++에서 기본 레이어를 생성하며 필요하면 파생 WBP가 각 컨테이너를 제공할 수 있다.
 */
UCLASS(Blueprintable)
class LASTFPS_API ULastFPSPrimaryGameLayout : public UPrimaryGameLayout
{
	GENERATED_BODY()

public:
	ULastFPSPrimaryGameLayout(const FObjectInitializer& ObjectInitializer);

	/** Modal→Menu→GameMenu 순 최상위 활성 위젯에 키보드 포커스 부여(HUD/Game 제외). */
	void RestoreFocusToTopActiveWidget();

	/** Modal/Menu/GameMenu 중 하나라도 활성 위젯이 있으면 true (Game/HUD 제외). 커서 소유 판정용. */
	bool HasActiveMenuLikeWidget();

	/** 커서 config 동기화를 위해 Modal/Menu/GameMenu 컨테이너의 표시위젯 변경 이벤트에 접근. */
	UCommonActivatableWidgetContainerBase* GetMenuLikeLayerContainer(int32 Index);
	static constexpr int32 NumMenuLikeLayers = 3;

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

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetContainerBase> LayerStack_Overlay;

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

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetContainerBase> AutoLayer_Overlay;
};
