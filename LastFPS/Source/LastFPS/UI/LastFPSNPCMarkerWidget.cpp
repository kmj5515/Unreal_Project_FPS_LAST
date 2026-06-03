#include "UI/LastFPSNPCMarkerWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"

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
