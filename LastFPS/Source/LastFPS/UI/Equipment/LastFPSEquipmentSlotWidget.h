#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "Data/Tables/LastFPSItemData.h"
#include "UI/Theme/LastFPSUIThemeReceiver.h"
#include "LastFPSEquipmentSlotWidget.generated.h"

class UImage;
class UTextBlock;
class ULastFPSButtonBase;
struct FStreamableHandle;

/** 슬롯 클릭 — 부모 화면이 아이템 선택 패널을 연다. */
DECLARE_DELEGATE_TwoParams(FOnEquipmentSlotClicked, ELastFPSEquipmentSlotType /*SlotType*/, int32 /*SlotIndex*/);

/**
 * WBP_EquipmentSlot 의 Parent — 장비 슬롯 1칸.
 *
 * 무기·리액터·외장·모듈 어느 카테고리에도 그대로 쓰는 범용 위젯이다. 자신이 어느 카테고리인지는
 * 표시 문구와 클릭 통보에만 쓰고, 장착 규칙은 전혀 알지 않는다(검증은 서브시스템이 단독으로 소유).
 *
 * Designer 바인딩(모두 선택): Image_Icon / Img_RarityBorder / Img_Empty / TB_SlotLabel / TB_ItemName / Button_Slot
 */
UCLASS()
class LASTFPS_API ULastFPSEquipmentSlotWidget : public UUserWidget, public ILastFPSUIThemeReceiver
{
	GENERATED_BODY()

public:
	virtual void ApplyUITheme(const ULastFPSUIThemeAsset& Theme) override;

	/** 이 슬롯이 어느 칸인지 지정한다. 표시 내용과 별개로 한 번만 설정한다. */
	void InitializeSlot(ELastFPSEquipmentSlotType InSlotType, int32 InSlotIndex, const FText& InSlotLabel);

	/** @param InItem 장착된 아이템 정의. @param InRowId 툴팁·선택 패널이 참조할 행 이름. */
	void SetEquipped(const FLastFPSItemData& InItem, FName InRowId);

	void SetEmpty();

	FName GetItemRowId() const { return ItemRowId; }
	ELastFPSEquipmentSlotType GetSlotType() const { return SlotType; }
	int32 GetSlotIndex() const { return SlotIndex; }

	FOnEquipmentSlotClicked OnSlotClicked;
	/** 우클릭 해제용. 부모 화면이 Unequip 을 호출한다. */
	FOnEquipmentSlotClicked OnSlotUnequipRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Image_Icon;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Img_RarityBorder;
	/** 빈 슬롯 표시용 (장착 시 숨김) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Img_Empty;
	/** 슬롯 이름 (예: "주무기", "외장 1") */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_SlotLabel;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_ItemName;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<ULastFPSButtonBase> Button_Slot;

private:
	void HandleSlotClicked();

	/** 등급 테두리를 다시 칠한다. 장착 시점과 테마 적용 시점이 달라 별도로 뺀다. */
	void RefreshRarityVisual();

	ELastFPSEquipmentSlotType SlotType = ELastFPSEquipmentSlotType::Weapon;
	int32 SlotIndex = INDEX_NONE;
	FName ItemRowId;

	/** 현재 장착 아이템의 등급. 테마가 늦게 와도 테두리를 다시 칠할 수 있게 들고 있는다. */
	ELastFPSItemRarity EquippedRarity = ELastFPSItemRarity::Common;

	/** 테마가 정하는 등급 발광 세기. 테마 지정 전에는 원래 색 그대로 표시한다. */
	float RarityGlowIntensity = 1.f;

	/**
	 * 진행 중인 아이콘 로드. 슬롯은 장착이 바뀔 때마다 재사용되므로, 새 요청 전에 이전 요청을 취소해야
	 * 늦게 도착한 이전 아이콘이 새 아이템 위에 덮이지 않는다.
	 */
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
