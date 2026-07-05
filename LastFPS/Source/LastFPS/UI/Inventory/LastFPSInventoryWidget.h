#pragma once

#include "UI/Framework/LastFPSContentScreenWidget.h"
#include "Data/Tables/LastFPSItemData.h"
#include "LastFPSInventoryWidget.generated.h"

class UDataTable;
class UPanelWidget;
class ULastFPSItemSlotWidget;
class ULastFPSEconomySubsystem;

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

	/** 아이템 정의 테이블 (RowType = FLastFPSItemData) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(RequiredAssetDataTags="RowStructure=/Script/LastFPS.LastFPSItemData"))
	TObjectPtr<UDataTable> ItemTable;

	/** 슬롯 1칸 위젯 클래스 (WBP_ItemSlot) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TSubclassOf<ULastFPSItemSlotWidget> SlotWidgetClass;

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

	/** 마우스가 올라가 있는 아이템 (F1 프리뷰 대상) */
	FName HoveredItemRowId;
};
