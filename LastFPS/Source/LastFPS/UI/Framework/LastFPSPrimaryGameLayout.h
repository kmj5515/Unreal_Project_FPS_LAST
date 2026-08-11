#pragma once

#include "GameplayTagContainer.h"
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

	/**
	 * 컷신 재생 동안 CinematicHiddenLayers 의 레이어를 숨긴다.
	 * 화면을 닫지 않고 표시만 접으므로 컷신이 끝나면 보던 화면으로 그대로 돌아온다.
	 * 게임 레이어(HUD)는 무전 자막처럼 남겨야 할 것이 있어 HUD 가 스스로 처리한다.
	 */
	void SetLayersHiddenForCinematic(bool bHidden);

protected:
	virtual void NativeOnInitialized() override;

	/**
	 * 컷신 중 숨길 레이어. 비워 두면 NativeOnInitialized 에서 기본값(게임 레이어를 뺀 전부)을 채운다.
	 * 태그를 쓰는 이유는 레이어가 늘어도 이 클래스를 고치지 않기 위해서다.
	 */
	UPROPERTY(EditDefaultsOnly, Category="UI|Cinematic", meta=(Categories="UI.Layer"))
	TArray<FGameplayTag> CinematicHiddenLayers;

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

	/** 컷신 진입 시 숨긴 레이어와 원래 표시 상태. 이미 숨겨져 있던 레이어를 켜 버리지 않으려고 남긴다. */
	TMap<FGameplayTag, ESlateVisibility> CinematicRestoreVisibilities;

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
