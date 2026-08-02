#pragma once

#include "UI/Framework/LastFPSContentScreenWidget.h"
#include "UI/Theme/LastFPSUIThemeReceiver.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "LastFPSEquipmentScreenWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class ULastFPSEquipmentSlotWidget;
class ULastFPSItemSelectionPanel;
class ULastFPSStatSummaryWidget;

/**
 * 한 카테고리를 어떤 이름과 어떤 슬롯 위젯으로 펼칠지 정하는 화면 저작 항목.
 *
 * 구조체 이름은 이미 저작된 화면 데이터와의 호환을 위해 유지한다(이름을 바꾸면 기존 값이 날아간다).
 */
USTRUCT()
struct FLastFPSEquipmentSectionLabels
{
	GENERATED_BODY()

	/**
	 * 슬롯별 표시 이름. 슬롯 수보다 적으면 남는 칸은 "이름 N" 형태로 자동 생성한다.
	 * 주무기/보조무기처럼 칸마다 뜻이 다른 카테고리를 데이터로 저작하기 위한 필드다.
	 */
	UPROPERTY(EditAnywhere, Category="Equipment")
	TArray<FText> SlotLabels;

	/** SlotLabels 가 모자랄 때 번호를 붙여 쓸 기본 이름 (예: "외장" → "외장 3") */
	UPROPERTY(EditAnywhere, Category="Equipment")
	FText FallbackLabel;

	/**
	 * 이 카테고리에만 쓸 슬롯 위젯. 비우면 화면의 기본 SlotWidgetClass 를 쓴다.
	 *
	 * 무기는 가로형 카드(160×90), 리액터·외장은 정사각(80×80)처럼 카테고리마다 규격이 달라서
	 * 위젯 클래스를 데이터로 갈아끼울 수 있어야 한다. 슬롯 위젯 자체는 여전히 카테고리를 모르며,
	 * 크기와 외형만 다른 파생 WBP 를 지정하는 방식이라 화면 코드에는 타입별 분기가 생기지 않는다.
	 */
	UPROPERTY(EditAnywhere, Category="Equipment")
	TSubclassOf<ULastFPSEquipmentSlotWidget> SlotWidgetClass;
};

/**
 * 통합 장비 화면 (ContentScreen) — 무기 2 / 리액터 1 / 외장 4 / 모듈 N 슬롯을 한 화면에서 관리한다.
 *
 * 슬롯 클릭 → 아이템 선택 패널 열기 → 확정 시 EquipmentSubsystem::TryEquip. 슬롯 우클릭 → Unequip.
 * 화면은 장착 규칙을 전혀 모르고, 서브시스템의 OnEquipmentChanged 를 구독해 표시만 다시 만든다.
 *
 * 캐릭터 3D 프리뷰는 레벨이 소유한 프리뷰 무대(ALastFPSPreviewStageActor)가 담당하며, 이 화면은 그 결과
 * RenderTarget 이미지를 배치할 자리(Img_CharacterPreview)만 노출한다.
 */
UCLASS()
class LASTFPS_API ULastFPSEquipmentScreenWidget
	: public ULastFPSContentScreenWidget, public ILastFPSUIThemeReceiver
{
	GENERATED_BODY()

public:
	virtual void ApplyUITheme(const ULastFPSUIThemeAsset& Theme) override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TSubclassOf<ULastFPSEquipmentSlotWidget> SlotWidgetClass;

	/** 카테고리별 슬롯 이름. 키가 없는 카테고리는 번호만 붙는다. */
	UPROPERTY(EditAnywhere, Category="Equipment")
	TMap<ELastFPSEquipmentSlotType, FLastFPSEquipmentSectionLabels> SectionLabels;

	// ── 슬롯이 채워질 패널 (레이아웃은 WBP 가 결정한다) ──
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_WeaponSlots;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_ReactorSlots;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_ExternalSlots;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_ModuleSlots;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<ULastFPSItemSelectionPanel> WBP_ItemSelection;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<ULastFPSStatSummaryWidget> WBP_StatSummary;

	/** 모듈 캐파 표시 (예: "8 / 10") */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_ModuleCapacity;
	/** 무기 슬롯 기준 DPS 합 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_DPS;

	/** 캐파 부족·슬롯 제한 등으로 장착이 거부됐을 때 — 연출용(선택, WBP 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category="Equipment")
	void OnEquipRejected();

private:
	void RebuildAll();
	/** 한 카테고리의 슬롯 위젯을 만들고 현재 장착 내용을 채운다. */
	void RebuildSection(ELastFPSEquipmentSlotType SlotType, UPanelWidget* Container);
	void UpdateSummary();
	void UpdateModuleCapacity();
	void UpdateWeaponDPS();

	FText ResolveSlotLabel(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex) const;

	/** 카테고리 전용 슬롯 위젯이 지정돼 있으면 그것을, 없으면 화면 기본값을 쓴다. */
	TSubclassOf<ULastFPSEquipmentSlotWidget> ResolveSlotWidgetClass(ELastFPSEquipmentSlotType SlotType) const;

	UFUNCTION()
	void HandleEquipmentChanged(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex);

	void HandleSlotClicked(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex);
	void HandleSlotUnequipRequested(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex);
	void HandleItemChosen(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex, FName ItemRowId);

	ULastFPSEquipmentSubsystem* GetEquipment() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSEquipmentSlotWidget>> SlotWidgets;

	/**
	 * 화면 진입 시 받은 테마. 슬롯 위젯은 그 뒤에 만들어지므로 보관했다가 생성 직후 적용한다.
	 * 테마 에셋의 수명은 설정이 쥐고 있어 이 화면이 소유권을 가질 이유가 없다.
	 */
	TWeakObjectPtr<const ULastFPSUIThemeAsset> CachedTheme;
};
