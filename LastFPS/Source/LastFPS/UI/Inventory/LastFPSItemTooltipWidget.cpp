#include "UI/Inventory/LastFPSItemTooltipWidget.h"

#include "Components/TextBlock.h"
#include "Data/Tables/LastFPSEquipmentStatTypes.h"
#include "Data/Tables/LastFPSModuleData.h"
#include "Inventory/LastFPSLoadoutSubsystem.h"
#include "Localization/LastFPSLocalization.h"

#include "Engine/GameInstance.h"

void ULastFPSItemTooltipWidget::SetupTooltip(const FLastFPSItemData& InItem, FName InRowId)
{
	if (TB_ItemName)
	{
		TB_ItemName->SetText(InItem.ItemName);
	}
	if (TB_Rarity)
	{
		TB_Rarity->SetText(FLastFPSLocalization::GetUIEnumText(
			StaticEnum<ELastFPSItemRarity>(),
			static_cast<int64>(InItem.Rarity)));
	}
	if (TB_Description)
	{
		TB_Description->SetText(InItem.Description);
	}

	// 스탯 줄은 모듈에만 있다. 무기/소모품/재료는 스탯 필드가 없어 숨긴다.
	FText StatsText;
	if (InItem.ItemType == ELastFPSItemType::Module)
	{
		if (const UGameInstance* GI = GetGameInstance())
		{
			if (const ULastFPSLoadoutSubsystem* Loadout = GI->GetSubsystem<ULastFPSLoadoutSubsystem>())
			{
				if (const FLastFPSModuleData* Module = Loadout->FindModule(InRowId))
				{
					TArray<FText> Lines;
					for (const FLastFPSModuleStatMod& Mod : Module->StatMods)
					{
						ELastFPSEquipmentStat EquipmentStat;
						if (LastFPSEquipmentStats::ToEquipmentStat(Mod.Stat, EquipmentStat))
						{
							Lines.Add(FText::Format(
								FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatLineFormat),
								LastFPSEquipmentStats::GetDisplayName(EquipmentStat),
								LastFPSEquipmentStats::FormatValue(EquipmentStat, Mod.Value, true)));
						}
					}
					StatsText = FText::Join(FText::FromString(TEXT("\n")), Lines);
				}
			}
		}
	}

	if (TB_Stats)
	{
		TB_Stats->SetText(StatsText);
		TB_Stats->SetVisibility(StatsText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	// "상세 보기(F1)" 힌트는 F1 프리뷰가 실제로 열리는 아이템(무기 + WeaponDefinition 연결)에만 보인다.
	if (TB_Detail)
	{
		const bool bHasDetailView =
			(InItem.ItemType == ELastFPSItemType::Weapon) && !InItem.WeaponDefinition.IsNull();
		TB_Detail->SetVisibility(bHasDetailView
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}
