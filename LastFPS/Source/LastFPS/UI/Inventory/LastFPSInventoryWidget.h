#pragma once

#include "UI/Framework/LastFPSContentScreenWidget.h"
#include "Data/Tables/LastFPSItemData.h"
#include "InputCoreTypes.h"
#include "LastFPSInventoryWidget.generated.h"

class UDataTable;
class UPanelWidget;
class ULastFPSItemSlotWidget;
class ULastFPSEconomySubsystem;
class ULastFPSWeaponPreviewWidget;

/**
 * 인벤토리 화면 — ContentScreen 크롬 위에 보유 아이템 슬롯 나열.
 * EconomySubsystem 의 보유 아이템(OwnedItems)을 ItemTable 정의로 해석해 보유한 만큼만 슬롯 생성
 * (빈 칸 패딩 없음). 보유 변동 시 OnInventoryChanged 로 자동 재구성.
 *
 * 카테고리 탭: AllowedTypes 로 표시할 ItemType 을 제한한다(비어 있으면 전체 표시).
 * 탭 버튼(WBP)이 SetAllowedTypes 를 호출해 런타임에 무기/소모품 등으로 전환한다.
 */
UCLASS()
class LASTFPS_API ULastFPSInventoryWidget : public ULastFPSContentScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * hover 중인 무기 아이템의 상세 프리뷰를 연다.
	 *
	 * OnKeyDown 이 아니라 Preview 단계에서 받는다. 슬롯이나 버튼처럼 자식 위젯이 포커스를 쥐고 있으면
	 * 일반 경로로는 이 위젯까지 키가 올라오지 않아, 툴팁이 떠 있는데도 눌러도 반응이 없다.
	 */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 상세 프리뷰를 여는 키. 하드코딩하지 않고 WBP 에서 바꿀 수 있게 둔다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	FKey WeaponPreviewKey = EKeys::F1;

	/** 아이템 정의 테이블 (RowType = FLastFPSItemData) */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> ItemTable;

	/** 슬롯 1칸 위젯 클래스 (WBP_ItemSlot) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TSubclassOf<ULastFPSItemSlotWidget> SlotWidgetClass;

	// F1 프리뷰의 위젯 클래스는 ScreenRegistry 의 UI.Screen.WeaponPreview 행이 정한다.
	// 여기에 또 두면 같은 지정이 두 곳으로 갈린다.

	/** 슬롯을 담을 컨테이너 (WrapBox 권장) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_Slots;

	/**
	 * 표시할 아이템 타입 화이트리스트. 비어 있으면 전체 표시.
	 * 탭 버튼(WBP)이 SetAllowedTypes 로 런타임 전환(예: 장비 탭={Weapon,Module}, 소모품 탭={Consumable,Material}).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TArray<ELastFPSItemType> AllowedTypes;

	/** 카테고리 필터 교체 후 즉시 재구성 — 탭 버튼 클릭에서 호출. */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetAllowedTypes(const TArray<ELastFPSItemType>& InTypes);

	/** 슬롯 전체 재구성 — 보유 아이템 갱신 후 호출 */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void RebuildInventory();

	/** 현재 마우스가 올라가 있는 아이템 행 이름 (없으면 NAME_None) — F1 프리뷰 등에서 사용. */
	FName GetHoveredItemRowId() const { return HoveredItemRowId; }

private:
	ULastFPSEconomySubsystem* GetEconomy() const;

	/** 보유 변동 시 슬롯 재구성 */
	UFUNCTION()
	void HandleInventoryChanged();

	/** 슬롯 hover 진입/이탈 — 현재 hover 아이템 추적 */
	void HandleSlotHovered(FName RowId);
	void HandleSlotUnhovered(FName RowId);

	/** 현재 hover 아이템이 무기면 프리뷰 오버레이를 Modal 레이어에 push. 열었으면 true. */
	bool TryOpenWeaponPreview();

	/** 마우스가 올라가 있는 아이템 (F1 프리뷰 대상) */
	FName HoveredItemRowId;
};
