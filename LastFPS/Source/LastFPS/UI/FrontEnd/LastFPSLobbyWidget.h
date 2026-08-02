#pragma once

#include "UI/Framework/LastFPSActivatableWidget.h"
#include "UI/Framework/LastFPSScreenTabHost.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "GameplayTagContainer.h"
#include "LastFPSLobbyWidget.generated.h"

class UCommonActivatableWidget;
class UCommonActivatableWidgetSwitcher;
class UCommonButtonBase;
class UCommonButtonGroupBase;
class UPanelWidget;
class UTextBlock;
class ULastFPSButtonBase;

/**
 * 아웃게임 껍데기(shell) — Menu Layer.
 *
 * 상단바를 단독으로 소유하고, 그 아래 스위처 안에서 콘텐츠 화면만 교체한다.
 * 화면마다 상단바를 따로 두면 탭을 누를 때마다 바가 통째로 다시 그려져 선택 상태가 유지되지 않는다.
 *
 * 탭 목록은 TabScreenTags 가 정하고 위젯은 ScreenRegistry 가 정하므로, 탭 추가는 데이터 두 곳
 * (레지스트리 행 + 태그 배열 항목)으로 끝난다. 레벨 이동(메인으로·던전)은 화면이 아니라서
 * 탭이 아닌 별도 버튼으로 남는다.
 *
 * Designer 바인딩(선택): HBox_Tabs(탭 버튼만 담는 패널) / Switcher_Content
 */
UCLASS()
class LASTFPS_API ULastFPSLobbyWidget : public ULastFPSActivatableWidget, public ILastFPSScreenTabHost
{
	GENERATED_BODY()

public:
	virtual bool HostsScreenTab(const FGameplayTag& InScreenTag) const override;
	virtual UCommonActivatableWidget* ShowScreenTab(const FGameplayTag& InScreenTag) override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	/**
	 * 이 껍데기 자신의 화면 태그. 레지스트리에서 "나를 호스트로 지정한" 탭을 모으는 기준이 된다.
	 * 탭 목록을 여기서 따로 저작하지 않으므로 버튼 배치와 태그 배열이 어긋날 일이 없다.
	 */
	UPROPERTY(EditAnywhere, Category="Lobby", meta=(Categories="UI.Screen"))
	FGameplayTag ScreenTag;

	/** 처음 열렸을 때 선택할 탭. 범위를 벗어나면 첫 번째 탭을 쓴다. */
	UPROPERTY(EditAnywhere, Category="Lobby", meta=(ClampMin="0"))
	int32 DefaultTabIndex = 0;

	/** 게임 화면과 프리뷰 무대를 오갈 때의 블렌드 시간. 0 이면 즉시 전환한다. */
	UPROPERTY(EditAnywhere, Category="Lobby", meta=(ClampMin="0.0", Units="s"))
	float PreviewViewBlendTime = 0.f;

	/**
	 * 이 껍데기가 떠 있는 동안 인게임 HUD 레이어를 접는다.
	 *
	 * HUD 위젯 하나를 지목하지 않고 레이어째 접는다. 조준선·퀘스트 추적처럼 그 레이어에 무엇이 더
	 * 올라오더라도 껍데기가 각각을 알 필요 없이 함께 가려진다.
	 */
	UPROPERTY(EditAnywhere, Category="Lobby")
	bool bHideGameLayerWhileOpen = true;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_PlayerName;

	/** 탭 버튼만 담는 패널. 메인으로·던전 같은 이동 버튼은 이 밖에 두어야 한다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> HBox_Tabs;

	/** 탭 화면이 들어갈 스위처. Activatable 수명주기를 함께 처리한다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetSwitcher> Switcher_Content;

private:
	void InitializeTabBar();

	/** 태그에 해당하는 화면을 스위처 안에 만든다. 이미 있으면 그것을 돌려준다. */
	UCommonActivatableWidget* FindOrCreateTabContent(const FGameplayTag& InScreenTag);

	UFUNCTION()
	void HandleTabSelected(UCommonButtonBase* AssociatedButton, int32 ButtonIndex);

	UFUNCTION() void HandleBackToMainClicked();

	/** 배타 선택 규칙을 대신 관리한다. 위젯이 아닌 UObject 라 GC 대상이므로 UPROPERTY 로 잡는다. */
	UPROPERTY(Transient)
	TObjectPtr<UCommonButtonGroupBase> TabGroup;

	/**
	 * 한 번 만든 탭 화면은 스위처 안에 남겨 두고 재사용한다.
	 * 탭을 오갈 때마다 다시 만들면 스크롤 위치·선택 같은 화면 상태가 매번 초기화된다.
	 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidget>> TabContents;

	/**
	 * 3D 프리뷰 무대를 켜 두는 구간을 표시한다.
	 *
	 * 무대는 레벨이 소유하므로 껍데기가 파괴할 수 없고, 파괴해서도 안 된다.
	 * 대신 이 껍데기가 떠 있는 동안만 켜 두어 메뉴가 닫힌 뒤 프레임 비용이 남지 않게 한다.
	 */
	void SetPreviewStageHeld(bool bHold);

	/** 무대에 선 캐릭터를 플레이어의 현재 모습으로 맞춘다. 장착이 바뀔 때마다 다시 부른다. */
	void RefreshPreviewSubject();

	/** 화면 정의가 지목한 프리뷰 시점으로 무대를 옮긴다. 지정이 없으면 원래 게임 화면으로 되돌린다. */
	void ApplyPreviewViewForScreen(const FGameplayTag& InScreenTag);

	/**
	 * 플레이어 뷰타깃을 무대와 원래 화면 사이에서 오간다.
	 * 시점 카메라를 켜는 것만으로는 화면이 바뀌지 않는다. 무대가 뷰타깃이어야 그 카메라가 화면이 된다.
	 */
	void SetStageAsViewTarget(bool bUseStage);

	/** 무대로 넘어가기 전의 뷰타깃. 메뉴를 닫거나 무대를 안 쓰는 탭으로 가면 여기로 돌아온다. */
	TWeakObjectPtr<AActor> PreviewPrevViewTarget;

	/** 인게임 HUD 레이어를 접고 되돌린다. 되돌릴 때는 접기 전 값을 그대로 복원한다. */
	void SetGameLayerHidden(bool bHidden);

	/** 접기 전 HUD 레이어의 표시 상태. 임의의 값으로 복원하면 히트테스트 설정이 바뀐다. */
	TOptional<ESlateVisibility> GameLayerPrevVisibility;

	UFUNCTION()
	void HandleEquipmentChanged(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex);

	/** 켜기·끄기 요청이 짝을 이루는지 보장한다. 중복 호출로 참조 수가 어긋나면 무대가 계속 켜져 있는다. */
	bool bHoldingPreviewStage = false;

	/** 초기 탭 선택 중에는 선택 통보를 무시한다(전환이 두 번 일어나는 것을 막는다). */
	bool bApplyingInitialTab = false;

	/** 레지스트리에서 TabOrder 순으로 모은 탭 화면. 인덱스가 곧 버튼 순서다. */
	TArray<FGameplayTag> TabScreenTags;
};
