#include "UI/Equipment/LastFPSStatEntryWidget.h"

#include "Components/TextBlock.h"
#include "UI/Theme/LastFPSUIThemeAsset.h"

void ULastFPSStatEntryWidget::SetEntry(const FText& InName, const FText& InValue, int32 InSign)
{
	CachedSign = InSign;

	if (TB_StatName)
	{
		TB_StatName->SetText(InName);
	}

	if (TB_StatValue)
	{
		TB_StatValue->SetText(InValue);
	}

	RefreshValueColor();
}

void ULastFPSStatEntryWidget::ApplyUITheme(const ULastFPSUIThemeAsset& Theme)
{
	PositiveValueColor = Theme.Palette.ValuePositive;
	NegativeValueColor = Theme.Palette.ValueNegative;
	NeutralValueColor = Theme.Palette.TextPrimary;

	if (TB_StatName)
	{
		// 스탯 행의 폰트는 WBP_StatEntry 디자이너 설정을 유지한다.
		// TB_StatName->SetFont(Theme.Typography.Label);
		TB_StatName->SetColorAndOpacity(FSlateColor(Theme.Palette.TextSecondary));
	}

	if (TB_StatValue)
	{
		// TB_StatValue->SetFont(Theme.Typography.Value);
	}

	RefreshValueColor();
}

void ULastFPSStatEntryWidget::RefreshValueColor()
{
	if (!TB_StatValue)
	{
		return;
	}

	const FLinearColor ValueColor =
		CachedSign > 0 ? PositiveValueColor :
		CachedSign < 0 ? NegativeValueColor : NeutralValueColor;
	TB_StatValue->SetColorAndOpacity(FSlateColor(ValueColor));
}
