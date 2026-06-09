#pragma once

#include "UI/LastFPSContentScreenWidget.h"
#include "LastFPSInventoryWidget.generated.h"

class UDataTable;
class UPanelWidget;
class ULastFPSItemSlotWidget;

/**
 * 인벤토리 화면 — ContentScreen 크롬 위에 고정 슬롯 그리드.
 * SlotCount 개의 슬롯을 생성한 뒤, ItemTable의 행 순서대로 앞 슬롯을 채우고 나머지는 빈 슬롯으로 유지.
 * 런타임 보유량/필터링은 추후 인벤토리 서브시스템 연동 후 RebuildInventory()에서 처리.
 */
UCLASS()
class LASTFPS_API ULastFPSInventoryWidget : public ULastFPSContentScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	/** 아이템 정의 테이블 (RowType = FLastFPSItemData) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(RequiredAssetDataTags="RowStructure=/Script/LastFPS.LastFPSItemData"))
	TObjectPtr<UDataTable> ItemTable;

	/** 슬롯 1칸 위젯 클래스 (WBP_ItemSlot) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TSubclassOf<ULastFPSItemSlotWidget> SlotWidgetClass;

	/** 슬롯을 담을 컨테이너 (WrapBox 권장) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_Slots;

	/** 고정 슬롯 수 — 아이템이 없는 슬롯은 빈 상태로 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(ClampMin=1))
	int32 SlotCount = 24;

	/** 슬롯 전체 재구성 — 테이블 교체 또는 아이템 갱신 후 호출 */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void RebuildInventory();
};
