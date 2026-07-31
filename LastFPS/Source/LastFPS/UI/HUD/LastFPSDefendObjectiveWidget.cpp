#include "UI/HUD/LastFPSDefendObjectiveWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void ULastFPSDefendObjectiveWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Bar_TargetHealth)
	{
		Bar_TargetHealth->SetFillColorAndOpacity(HealthFillColor);
	}
}

void ULastFPSDefendObjectiveWidget::SetupObjective(const FText& InLabel, const float InTotalSeconds)
{
	TotalSeconds = FMath::Max(InTotalSeconds, 0.f);

	if (Text_Label)
	{
		Text_Label->SetText(InLabel);
	}

	UpdateDisplay(0.f, -1.f);
}

void ULastFPSDefendObjectiveWidget::UpdateDisplay(const float Progress01, const float Health01)
{
	const float Clamped = FMath::Clamp(Progress01, 0.f, 1.f);
	const float Remaining = TotalSeconds * (1.f - Clamped);

	if (Text_RemainingTime)
	{
		Text_RemainingTime->SetText(FormatRemaining(Remaining));
	}

	ApplyTargetHealth(Health01);
	OnDefendDisplayUpdated(Remaining, Health01);
}

void ULastFPSDefendObjectiveWidget::ApplyTargetHealth(const float Health01)
{
	if (!Bar_TargetHealth)
	{
		return;
	}

	// 음수는 "지킬 대상이 없다" — 체력 표시 자체를 숨긴다.
	if (Health01 < 0.f)
	{
		Bar_TargetHealth->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const float Clamped = FMath::Clamp(Health01, 0.f, 1.f);
	Bar_TargetHealth->SetVisibility(ESlateVisibility::HitTestInvisible);
	Bar_TargetHealth->SetPercent(Clamped);
	Bar_TargetHealth->SetFillColorAndOpacity(
		Clamped <= LowHealthThreshold ? HealthLowFillColor : HealthFillColor);
}

FText ULastFPSDefendObjectiveWidget::FormatRemaining(const float RemainingSeconds)
{
	// 올림으로 표기해야 "00:00"이 뜬 뒤에도 잠깐 남아 있는 상황이 생기지 않는다.
	const int32 TotalWholeSeconds = FMath::Max(FMath::CeilToInt(RemainingSeconds), 0);
	const int32 Minutes = TotalWholeSeconds / 60;
	const int32 Seconds = TotalWholeSeconds % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}
