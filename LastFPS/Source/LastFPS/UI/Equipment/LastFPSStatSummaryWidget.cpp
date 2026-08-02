#include "UI/Equipment/LastFPSStatSummaryWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "UI/Equipment/LastFPSStatEntryWidget.h"
#include "UI/Theme/LastFPSUIThemeAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSStatSummary, Log, All);

void ULastFPSStatSummaryWidget::SetTotals(const FLastFPSEquipmentStatTotals& InTotals)
{
	// 표시할 스탯만 먼저 추린다. 행 위젯 개수를 한 번에 맞추기 위해서다.
	struct FVisibleStat
	{
		ELastFPSEquipmentStat Stat;
		float Value;
	};

	TArray<FVisibleStat, TInlineAllocator<static_cast<int32>(ELastFPSEquipmentStat::Count)>> VisibleStats;

	for (int32 StatIndex = 0; StatIndex < static_cast<int32>(ELastFPSEquipmentStat::Count); ++StatIndex)
	{
		const ELastFPSEquipmentStat Stat = static_cast<ELastFPSEquipmentStat>(StatIndex);
		const float Value = InTotals.GetStat(Stat);
		if (FMath::IsNearlyZero(Value))
		{
			continue;
		}

		VisibleStats.Add({ Stat, Value });
	}

	const int32 ReadyCount = EnsureEntryCount(VisibleStats.Num());

	for (int32 EntryIndex = 0; EntryIndex < EntryWidgets.Num(); ++EntryIndex)
	{
		ULastFPSStatEntryWidget* Entry = EntryWidgets[EntryIndex];
		if (!Entry)
		{
			continue;
		}

		// 남는 행은 파괴하지 않고 숨겨 둔다. 다음 갱신에서 다시 쓰기 위해서다.
		if (EntryIndex >= ReadyCount)
		{
			Entry->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		const FVisibleStat& VisibleStat = VisibleStats[EntryIndex];
		Entry->SetEntry(
			LastFPSEquipmentStats::GetDisplayName(VisibleStat.Stat),
			LastFPSEquipmentStats::FormatValue(VisibleStat.Stat, VisibleStat.Value, /*bShowSign=*/true),
			VisibleStat.Value > 0.f ? 1 : -1);
		Entry->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(
			ReadyCount > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

int32 ULastFPSStatSummaryWidget::EnsureEntryCount(int32 RequiredCount)
{
	if (RequiredCount <= EntryWidgets.Num())
	{
		return RequiredCount;
	}

	// 설정 누락은 고쳐질 때까지 갱신마다 반복되므로 한 번만 알린다.
	if (!Box_Stats || !StatEntryClass)
	{
		if (!bLoggedMissingSetup)
		{
			bLoggedMissingSetup = true;
			UE_LOG(LogLastFPSStatSummary, Warning,
				TEXT("%s: 스탯 행을 만들 수 없습니다(Box_Stats=%s, StatEntryClass=%s). WBP 설정을 확인하세요."),
				*GetName(),
				Box_Stats ? TEXT("OK") : TEXT("없음"),
				StatEntryClass ? TEXT("OK") : TEXT("없음"));
		}
		return EntryWidgets.Num();
	}

	while (EntryWidgets.Num() < RequiredCount)
	{
		ULastFPSStatEntryWidget* Entry =
			CreateWidget<ULastFPSStatEntryWidget>(GetOwningPlayer(), StatEntryClass);
		if (!Entry)
		{
			UE_LOG(LogLastFPSStatSummary, Warning,
				TEXT("%s: %s 행 위젯 생성에 실패했습니다."),
				*GetName(), *GetNameSafe(StatEntryClass));
			break;
		}

		Box_Stats->AddChild(Entry);
		EntryWidgets.Add(Entry);

		// 화면 전체 테마 적용은 이 행이 생기기 전에 끝났으므로 여기서 따라잡는다.
		if (const ULastFPSUIThemeAsset* Theme = CachedTheme.Get())
		{
			Entry->ApplyUITheme(*Theme);
		}
	}

	return FMath::Min(RequiredCount, EntryWidgets.Num());
}

void ULastFPSStatSummaryWidget::ApplyUITheme(const ULastFPSUIThemeAsset& Theme)
{
	CachedTheme = &Theme;

	if (TB_Empty)
	{
		TB_Empty->SetFont(Theme.Typography.Caption);
		TB_Empty->SetColorAndOpacity(FSlateColor(Theme.Palette.TextMuted));
	}

	// 이미 만들어 둔 행에도 반영한다(테마 교체·핫리로드 대응).
	for (ULastFPSStatEntryWidget* Entry : EntryWidgets)
	{
		if (Entry)
		{
			Entry->ApplyUITheme(Theme);
		}
	}
}
