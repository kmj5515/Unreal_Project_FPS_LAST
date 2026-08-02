#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LastFPSWeaponSlotPresenter.generated.h"

class UPanelWidget;
class UWeaponComponent;
class ULastFPSWeaponSlotWidget;

/**
 * HUD 무기 슬롯 표기를 관리하는 Presenter.
 *
 * UWeaponComponent 의 슬롯 구성·활성 슬롯 이벤트를 받아 슬롯 위젯 목록을 만들고 갱신한다.
 * 구성이 바뀔 때만 위젯을 재생성하고, 활성 슬롯만 바뀐 경우에는 강조 표시만 바꿔
 * 1·2 키를 빠르게 눌러도 위젯이 매번 새로 생성되지 않게 한다.
 */
UCLASS()
class LASTFPS_API ULastFPSWeaponSlotPresenter final : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @param InSlotContainer 슬롯 위젯을 담을 패널. null 이면 이 Presenter 는 아무것도 하지 않는다.
	 * @param InSlotWidgetClass 생성할 슬롯 위젯 클래스(WBP_HUDWeaponSlot).
	 */
	void Initialize(UPanelWidget* InSlotContainer, TSubclassOf<ULastFPSWeaponSlotWidget> InSlotWidgetClass);

	void BindToWeaponComponent(UWeaponComponent* InWeaponComponent);

	/** 바인딩 해제 및 생성한 슬롯 위젯 정리 */
	void Reset();

private:
	UFUNCTION()
	void HandleLoadoutChanged();

	UFUNCTION()
	void HandleActiveSlotChanged(int32 ActiveSlotIndex);

	/** 슬롯 개수가 달라졌을 때만 위젯을 다시 만들고, 그 외에는 내용만 갱신한다. */
	void RebuildSlots();
	void RefreshActiveHighlight();

	UPROPERTY(Transient)
	TWeakObjectPtr<UPanelWidget> SlotContainer;

	UPROPERTY(Transient)
	TSubclassOf<ULastFPSWeaponSlotWidget> SlotWidgetClass;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSWeaponSlotWidget>> SlotWidgets;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWeaponComponent> WeaponComponent;
};
