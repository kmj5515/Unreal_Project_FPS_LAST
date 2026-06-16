#pragma once

#include "UI/LastFPSContentScreenWidget.h"
#include "LastFPSModuleScreenWidget.generated.h"

class UDataTable;
class UPanelWidget;
class UTextBlock;
class ULastFPSModuleEntryWidget;
class ULastFPSModuleSlotWidget;
class ULastFPSLoadoutSubsystem;
class ULastFPSEconomySubsystem;
struct FLastFPSItemData;
struct FLastFPSModuleStatTotals;

/**
 * 모듈 장착 화면 (ContentScreen) — 보유 모듈 목록(좌) + 장착 슬롯(우) + 캐파/스탯 미리보기.
 *
 * 보유 모듈 클릭 → 빈 슬롯에 장착(LoadoutSubsystem::TryEquip). 슬롯 클릭 → 해제(Unequip).
 * LoadoutSubsystem::OnLoadoutChanged / Economy::OnInventoryChanged 구독해 자동 재구성.
 * 모듈 표시(아이콘/이름/희귀도)는 ItemTable(DT_ItemData), 스탯/캐파는 LoadoutSubsystem(DT_ModuleData).
 */
UCLASS()
class LASTFPS_API ULastFPSModuleScreenWidget : public ULastFPSContentScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 모듈 표시용 아이템 정의 테이블 (RowType = FLastFPSItemData) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Module", meta=(RequiredAssetDataTags="RowStructure=/Script/LastFPS.LastFPSItemData"))
	TObjectPtr<UDataTable> ItemTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Module")
	TSubclassOf<ULastFPSModuleEntryWidget> EntryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Module")
	TSubclassOf<ULastFPSModuleSlotWidget> SlotWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_OwnedModules;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_Slots;
	/** 캐파 표시 (예: "8 / 10") */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Capacity;
	/** 장착 모듈 합산 스탯 보정 미리보기 (멀티라인) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_StatPreview;
	/** 보유 모듈이 없을 때 안내 (선택) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Empty;

	/** 캐파/슬롯 부족으로 장착이 거부됐을 때 — 연출용(선택, WBP 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category="Module")
	void OnEquipRejected();

private:
	void Rebuild();
	void RebuildOwned(ULastFPSLoadoutSubsystem* Loadout, ULastFPSEconomySubsystem* Econ);
	void RebuildSlots(ULastFPSLoadoutSubsystem* Loadout);
	void UpdatePreview(ULastFPSLoadoutSubsystem* Loadout);

	UFUNCTION() void HandleLoadoutChanged();
	UFUNCTION() void HandleInventoryChanged();

	void HandleEquipClicked(FName RowId);
	void HandleSlotClicked(int32 SlotIndex);

	int32 FindFirstEmptySlot(ULastFPSLoadoutSubsystem* Loadout) const;
	const FLastFPSItemData* FindItem(FName RowId) const;

	ULastFPSLoadoutSubsystem* GetLoadout() const;
	ULastFPSEconomySubsystem* GetEconomy() const;
};
