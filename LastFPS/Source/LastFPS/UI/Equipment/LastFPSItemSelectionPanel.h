#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "LastFPSItemSelectionPanel.generated.h"

class UPanelWidget;
class UTextBlock;
class ULastFPSButtonBase;
class ULastFPSItemSlotWidget;

/** 선택 확정 — 부모 화면이 실제 장착을 수행한다. */
DECLARE_DELEGATE_ThreeParams(
	FOnEquipmentItemChosen, ELastFPSEquipmentSlotType /*SlotType*/, int32 /*SlotIndex*/, FName /*ItemRowId*/);

/**
 * WBP_ItemSelectionPanel 의 Parent — 슬롯을 클릭했을 때 열리는 아이템 선택 패널.
 *
 * 보유 아이템 중 해당 슬롯 카테고리에 맞는 것만 걸러 보여주고, 선택한 아이템의 장착 전후 스탯 차이를
 * 미리 보여준다. 장착 가능 여부와 스탯 계산은 모두 EquipmentSubsystem 에 묻고 스스로 규칙을 두지 않는다.
 */
UCLASS()
class LASTFPS_API ULastFPSItemSelectionPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 대상 슬롯을 지정하고 목록을 채운다. 패널을 열 때마다 호출한다. */
	void OpenForSlot(ELastFPSEquipmentSlotType InSlotType, int32 InSlotIndex);

	void Close();

	FOnEquipmentItemChosen OnItemChosen;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TSubclassOf<ULastFPSItemSlotWidget> EntryWidgetClass;

	/** 현재 대상 슬롯에 장착할 수 없는 후보의 표시 투명도다. */
	UPROPERTY(EditDefaultsOnly, Category="Equipment", meta=(ClampMin="0.0", ClampMax="1.0"))
	float UnequippableOpacity = 0.4f;

	/** 후보 아이템 카드가 채워질 패널 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_Candidates;
	/** 장착 전후 비교 결과(멀티라인). 변화가 없으면 비운다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_StatComparison;
	/** 보유한 후보가 없을 때 안내 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Empty;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<ULastFPSButtonBase> Button_Confirm;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<ULastFPSButtonBase> Button_Cancel;

	/** 후보가 하나도 없을 때 표시할 문구 */
	UPROPERTY(EditDefaultsOnly, Category="Equipment")
	FText EmptyCandidateText;

private:
	void RebuildCandidates();
	void UpdateComparison();

	void HandleEntrySelected(FName ItemRowId);
	void HandleEntryChosen(FName ItemRowId);
	void HandleConfirmClicked();
	void HandleCancelClicked();

	ULastFPSEquipmentSubsystem* GetEquipment() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSItemSlotWidget>> EntryWidgets;

	ELastFPSEquipmentSlotType TargetSlotType = ELastFPSEquipmentSlotType::Weapon;
	int32 TargetSlotIndex = INDEX_NONE;
	FName SelectedItemRowId;
};
