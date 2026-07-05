#include "UI/Inventory/LastFPSItemTooltipWidget.h"

#include "Components/TextBlock.h"
#include "Data/Tables/LastFPSModuleData.h"
#include "Inventory/LastFPSLoadoutSubsystem.h"

#include "Engine/GameInstance.h"

void ULastFPSItemTooltipWidget::SetupTooltip(const FLastFPSItemData& InItem, FName InRowId)
{
	if (TB_ItemName)
	{
		TB_ItemName->SetText(InItem.ItemName);
	}
	if (TB_Rarity)
	{
		TB_Rarity->SetText(UEnum::GetDisplayValueAsText(InItem.Rarity));
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
					FString Lines;
					for (const FLastFPSModuleStatMod& Mod : Module->StatMods)
					{
						const FText StatName = UEnum::GetDisplayValueAsText(Mod.Stat);
						const TCHAR* Sign = (Mod.Value >= 0.f) ? TEXT("+") : TEXT("");
						if (!Lines.IsEmpty())
						{
							Lines += TEXT("\n");
						}
						Lines += FString::Printf(TEXT("%s %s%g"), *StatName.ToString(), Sign, Mod.Value);
					}
					StatsText = FText::FromString(Lines);
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
}
