#include "UI/Equipment/LastFPSEquipmentScreenWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Inventory/LastFPSLoadoutSubsystem.h"
#include "Localization/LastFPSLocalization.h"
#include "UI/Equipment/LastFPSEquipmentSlotWidget.h"
#include "UI/Equipment/LastFPSItemSelectionPanel.h"
#include "UI/Equipment/LastFPSStatSummaryWidget.h"
#include "UI/Theme/LastFPSUIThemeAsset.h"

#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBoxSlot.h"

namespace
{
	/**
	 * 컨테이너가 슬롯 위젯을 늘리지 않게 한다.
	 *
	 * 슬롯 WBP 는 루트 SizeBox 로 자기 크기를 선언한다(무기 160×90, 리액터·외장 80×80).
	 * AddChild 기본 정렬이 Fill 이라 그대로 두면 그 선언이 무시되고 패널 폭까지 늘어난다.
	 * 크기를 코드가 정하지 않고 위젯이 정한 값을 지키게 하는 것이 목적이다.
	 *
	 * @param Padding 슬롯 사이 간격. 테마의 RowSpacing 을 그대로 받는다.
	 */
	void ApplyIntrinsicSizeToSlot(UPanelSlot* PanelSlot, const float Padding)
	{
		const FMargin SlotPadding(0.f, 0.f, Padding, Padding);
		// 남는 공간을 나눠 갖지 않고 자식이 원하는 만큼만 차지한다.
		const FSlateChildSize AutoSize(ESlateSizeRule::Automatic);

		if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(PanelSlot))
		{
			VerticalSlot->SetSize(AutoSize);
			VerticalSlot->SetHorizontalAlignment(HAlign_Left);
			VerticalSlot->SetVerticalAlignment(VAlign_Top);
			VerticalSlot->SetPadding(SlotPadding);
			return;
		}

		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(PanelSlot))
		{
			HorizontalSlot->SetSize(AutoSize);
			HorizontalSlot->SetHorizontalAlignment(HAlign_Left);
			HorizontalSlot->SetVerticalAlignment(VAlign_Top);
			HorizontalSlot->SetPadding(SlotPadding);
			return;
		}

		if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(PanelSlot))
		{
			// WrapBox 는 Size 규칙 대신 이 값으로 늘림 여부를 정한다.
			WrapSlot->SetFillEmptySpace(false);
			WrapSlot->SetHorizontalAlignment(HAlign_Left);
			WrapSlot->SetVerticalAlignment(VAlign_Top);
			WrapSlot->SetPadding(SlotPadding);
		}
	}
}

void ULastFPSEquipmentScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULastFPSEquipmentSubsystem* Equipment = GetEquipment())
	{
		Equipment->OnEquipmentChanged.AddUniqueDynamic(
			this, &ULastFPSEquipmentScreenWidget::HandleEquipmentChanged);
	}

	if (WBP_ItemSelection)
	{
		WBP_ItemSelection->OnItemChosen.BindUObject(
			this, &ULastFPSEquipmentScreenWidget::HandleItemChosen);
	}

	RebuildAll();
}

void ULastFPSEquipmentScreenWidget::NativeDestruct()
{
	if (ULastFPSEquipmentSubsystem* Equipment = GetEquipment())
	{
		Equipment->OnEquipmentChanged.RemoveDynamic(
			this, &ULastFPSEquipmentScreenWidget::HandleEquipmentChanged);
	}

	if (WBP_ItemSelection)
	{
		WBP_ItemSelection->OnItemChosen.Unbind();
	}

	Super::NativeDestruct();
}

void ULastFPSEquipmentScreenWidget::ApplyUITheme(const ULastFPSUIThemeAsset& Theme)
{
	CachedTheme = &Theme;

	if (TB_DPS)
	{
		TB_DPS->SetFont(Theme.Typography.Value);
		TB_DPS->SetColorAndOpacity(FSlateColor(Theme.Palette.AccentPrimary));
	}

	if (TB_ModuleCapacity)
	{
		TB_ModuleCapacity->SetFont(Theme.Typography.Value);
		TB_ModuleCapacity->SetColorAndOpacity(FSlateColor(Theme.Palette.TextPrimary));
	}

	// 이미 만들어 둔 슬롯에도 반영한다(테마 교체·핫리로드 대응).
	for (ULastFPSEquipmentSlotWidget* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ApplyUITheme(Theme);
		}
	}
}

ULastFPSEquipmentSubsystem* ULastFPSEquipmentScreenWidget::GetEquipment() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>() : nullptr;
}

FText ULastFPSEquipmentScreenWidget::ResolveSlotLabel(
	const ELastFPSEquipmentSlotType SlotType, const int32 SlotIndex) const
{
	const FLastFPSEquipmentSectionLabels* Labels = SectionLabels.Find(SlotType);
	if (!Labels)
	{
		return FText::AsNumber(SlotIndex + 1);
	}

	if (Labels->SlotLabels.IsValidIndex(SlotIndex))
	{
		return Labels->SlotLabels[SlotIndex];
	}

	// 칸마다 이름을 다 저작하지 않아도 되도록 남는 칸은 번호를 붙여 만든다.
	return FText::Format(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::IndexedLabelFormat),
		Labels->FallbackLabel,
		FText::AsNumber(SlotIndex + 1));
}

TSubclassOf<ULastFPSEquipmentSlotWidget> ULastFPSEquipmentScreenWidget::ResolveSlotWidgetClass(
	const ELastFPSEquipmentSlotType SlotType) const
{
	if (const FLastFPSEquipmentSectionLabels* Section = SectionLabels.Find(SlotType))
	{
		if (Section->SlotWidgetClass)
		{
			return Section->SlotWidgetClass;
		}
	}

	return SlotWidgetClass;
}

void ULastFPSEquipmentScreenWidget::RebuildAll()
{
	SlotWidgets.Reset();

	RebuildSection(ELastFPSEquipmentSlotType::Weapon, Box_WeaponSlots);
	RebuildSection(ELastFPSEquipmentSlotType::Reactor, Box_ReactorSlots);
	RebuildSection(ELastFPSEquipmentSlotType::ExternalComponent, Box_ExternalSlots);
	RebuildSection(ELastFPSEquipmentSlotType::Module, Box_ModuleSlots);

	UpdateSummary();
	UpdateModuleCapacity();
	UpdateWeaponDPS();
}

void ULastFPSEquipmentScreenWidget::RebuildSection(
	const ELastFPSEquipmentSlotType SlotType, UPanelWidget* Container)
{
	if (!Container)
	{
		return;
	}

	Container->ClearChildren();

	ULastFPSEquipmentSubsystem* Equipment = GetEquipment();
	const TSubclassOf<ULastFPSEquipmentSlotWidget> ResolvedSlotClass = ResolveSlotWidgetClass(SlotType);
	if (!Equipment || !ResolvedSlotClass)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	const UDataTable* ItemTable =
		GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item) : nullptr;

	static const FString Context(TEXT("ULastFPSEquipmentScreenWidget::RebuildSection"));

	const int32 SlotCount = Equipment->GetSlotCount(SlotType);
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		ULastFPSEquipmentSlotWidget* SlotWidget =
			CreateWidget<ULastFPSEquipmentSlotWidget>(GetOwningPlayer(), ResolvedSlotClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->InitializeSlot(SlotType, SlotIndex, ResolveSlotLabel(SlotType, SlotIndex));

		// 화면 전체 테마 적용은 슬롯이 생기기 전에 끝났으므로 여기서 따라잡는다.
		if (const ULastFPSUIThemeAsset* Theme = CachedTheme.Get())
		{
			SlotWidget->ApplyUITheme(*Theme);
		}

		const FName EquippedRowId = Equipment->GetEquippedItem(SlotType, SlotIndex);
		const FLastFPSItemData* Item = (ItemTable && !EquippedRowId.IsNone())
			? ItemTable->FindRow<FLastFPSItemData>(EquippedRowId, Context, /*bWarnIfRowMissing=*/false)
			: nullptr;

		if (Item)
		{
			SlotWidget->SetEquipped(*Item, EquippedRowId);
		}
		else
		{
			SlotWidget->SetEmpty();
		}

		SlotWidget->OnSlotClicked.BindUObject(
			this, &ULastFPSEquipmentScreenWidget::HandleSlotClicked);
		SlotWidget->OnSlotUnequipRequested.BindUObject(
			this, &ULastFPSEquipmentScreenWidget::HandleSlotUnequipRequested);

		UPanelSlot* AddedSlot = Container->AddChild(SlotWidget);

		// 간격은 테마가 정한다. 테마가 없으면 붙여서 배치한다.
		const ULastFPSUIThemeAsset* Theme = CachedTheme.Get();
		ApplyIntrinsicSizeToSlot(AddedSlot, Theme ? Theme->Metrics.RowSpacing : 0.f);

		SlotWidgets.Add(SlotWidget);
	}
}

void ULastFPSEquipmentScreenWidget::UpdateSummary()
{
	if (!WBP_StatSummary)
	{
		return;
	}

	if (const ULastFPSEquipmentSubsystem* Equipment = GetEquipment())
	{
		WBP_StatSummary->SetTotals(Equipment->ComputeTotals());
	}
}

void ULastFPSEquipmentScreenWidget::UpdateModuleCapacity()
{
	if (!TB_ModuleCapacity)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULastFPSLoadoutSubsystem* Loadout =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSLoadoutSubsystem>() : nullptr;
	if (!Loadout)
	{
		TB_ModuleCapacity->SetText(FText::GetEmpty());
		return;
	}

	TB_ModuleCapacity->SetText(FText::Format(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::RatioFormat),
		FText::AsNumber(Loadout->GetUsedCapacity()),
		FText::AsNumber(Loadout->GetMaxCapacity())));
}

void ULastFPSEquipmentScreenWidget::UpdateWeaponDPS()
{
	if (!TB_DPS)
	{
		return;
	}

	ULastFPSEquipmentSubsystem* Equipment = GetEquipment();
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	const UDataTable* ItemTable =
		GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item) : nullptr;

	if (!Equipment || !ItemTable)
	{
		TB_DPS->SetText(FText::GetEmpty());
		return;
	}

	static const FString Context(TEXT("ULastFPSEquipmentScreenWidget::UpdateWeaponDPS"));

	// 표시용 스펙 시트(FLastFPSWeaponDisplayStats)를 기준으로 계산한다.
	// 실제 전투 수치는 DT_WeaponBalance 가 소유하므로, 이 값은 무기 비교용 지표로만 쓴다.
	float TotalDPS = 0.f;
	const int32 SlotCount = Equipment->GetSlotCount(ELastFPSEquipmentSlotType::Weapon);
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const FName RowId = Equipment->GetEquippedItem(ELastFPSEquipmentSlotType::Weapon, SlotIndex);
		if (RowId.IsNone())
		{
			continue;
		}

		const FLastFPSItemData* Item =
			ItemTable->FindRow<FLastFPSItemData>(RowId, Context, /*bWarnIfRowMissing=*/false);
		if (!Item)
		{
			continue;
		}

		const FLastFPSWeaponDisplayStats& Stats = Item->WeaponStats;
		if (Stats.FireRate <= 0.f)
		{
			continue;
		}

		const float AverageDamage = (Stats.MinDamage + Stats.MaxDamage) * 0.5f;
		TotalDPS += AverageDamage / Stats.FireRate;
	}

	FNumberFormattingOptions Options;
	Options.SetMaximumFractionalDigits(0);
	Options.SetUseGrouping(true);
	TB_DPS->SetText(FText::AsNumber(TotalDPS, &Options));
}

void ULastFPSEquipmentScreenWidget::HandleEquipmentChanged(
	ELastFPSEquipmentSlotType SlotType, int32 SlotIndex)
{
	// 카테고리 간 스탯 합계와 DPS 가 서로 영향을 주므로 부분 갱신보다 전체 재구성이 안전하다.
	// 화면이 열려 있는 동안에만 일어나는 조작이라 비용도 문제되지 않는다.
	RebuildAll();
}

void ULastFPSEquipmentScreenWidget::HandleSlotClicked(
	const ELastFPSEquipmentSlotType SlotType, const int32 SlotIndex)
{
	if (WBP_ItemSelection)
	{
		WBP_ItemSelection->OpenForSlot(SlotType, SlotIndex);
	}
}

void ULastFPSEquipmentScreenWidget::HandleSlotUnequipRequested(
	const ELastFPSEquipmentSlotType SlotType, const int32 SlotIndex)
{
	if (ULastFPSEquipmentSubsystem* Equipment = GetEquipment())
	{
		Equipment->Unequip(SlotType, SlotIndex);
	}
}

void ULastFPSEquipmentScreenWidget::HandleItemChosen(
	const ELastFPSEquipmentSlotType SlotType, const int32 SlotIndex, const FName ItemRowId)
{
	ULastFPSEquipmentSubsystem* Equipment = GetEquipment();
	if (!Equipment)
	{
		return;
	}

	if (!Equipment->TryEquip(SlotType, SlotIndex, ItemRowId))
	{
		OnEquipRejected();
		return;
	}

	// 장착이 끝나면 선택 패널을 닫는다. 재구성은 OnEquipmentChanged 가 이미 처리한다.
	if (WBP_ItemSelection)
	{
		WBP_ItemSelection->Close();
	}
}
