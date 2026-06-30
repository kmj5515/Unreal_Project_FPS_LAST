#include "UI/HUD/LastFPSNPCMarkerWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/ProgressBar.h"

void ULastFPSNPCMarkerWidget::SetNPCInfo(const FText& InName, const FText& InRole)
{
	if (TB_NPCName) TB_NPCName->SetText(InName);
	if (TB_NPCRole) TB_NPCRole->SetText(InRole);
}

void ULastFPSNPCMarkerWidget::SetInteractionHintVisible(bool bVisible)
{
	if (InteractionHint)
	{
		InteractionHint->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void ULastFPSNPCMarkerWidget::SetInteractionLabel(const FText& InLabel)
{
	if (TB_InteractionLabel) TB_InteractionLabel->SetText(InLabel);
}

void ULastFPSNPCMarkerWidget::SetInteractionProgress(float Progress)
{
	Progress = FMath::Clamp(Progress, 0.f, 1.f);
	const bool bActive = Progress > 0.f;

	if (HoldGaugeRoot)
	{
		HoldGaugeRoot->SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (PB_HoldGauge)
	{
		PB_HoldGauge->SetPercent(Progress);
	}

	// 원형 게이지 등 커스텀 비주얼은 디자이너가 BP에서 구성.
	OnInteractionProgressChanged(Progress);
}
