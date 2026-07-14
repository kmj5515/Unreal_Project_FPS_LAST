#include "UI/HUD/LastFPSObjectiveMarkerEntryWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"

void ULastFPSObjectiveMarkerEntryWidget::UpdateMarker(float DistanceMeters, bool bOffScreen, float ArrowAngleDeg, const FText& Label)
{
	if (Text_Distance)
	{
		// "45m" 형태 — 정수 미터.
		const FText MetersText = FText::Format(
			NSLOCTEXT("LastFPS", "Quest_MarkerDistance", "{0}m"),
			FText::AsNumber(FMath::RoundToInt(DistanceMeters)));
		Text_Distance->SetText(MetersText);
	}

	if (Text_Label)
	{
		Text_Label->SetText(Label);
	}

	if (Arrow)
	{
		if (bOffScreen)
		{
			Arrow->SetVisibility(ESlateVisibility::HitTestInvisible);
			Arrow->SetRenderTransformAngle(ArrowAngleDeg);
		}
		else
		{
			Arrow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	OnMarkerUpdated(DistanceMeters, bOffScreen, ArrowAngleDeg);
}
