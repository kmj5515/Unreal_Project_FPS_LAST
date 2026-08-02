#include "UI/HUD/LastFPSObjectiveMarkerEntryWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Localization/LastFPSLocalization.h"

void ULastFPSObjectiveMarkerEntryWidget::UpdateMarker(const FLastFPSObjectiveMarkerDisplay& Display)
{
	// 경유 지점도 도달 목표와 동일하게 거리/라벨을 표시한다(안내 지점은 항상 한 번에 하나).
	if (Text_Distance)
	{
		// "45m" 형태 — 정수 미터.
		const FText MetersText = FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::DistanceMetersFormat),
			FText::AsNumber(FMath::RoundToInt(Display.DistanceMeters)));
		Text_Distance->SetText(MetersText);
	}

	if (Text_Label)
	{
		Text_Label->SetText(Display.Label);
	}

	if (Arrow)
	{
		if (Display.bOffScreen)
		{
			Arrow->SetVisibility(ESlateVisibility::HitTestInvisible);
			Arrow->SetRenderTransformAngle(Display.ArrowAngleDeg);
		}
		else
		{
			Arrow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Icon_Destination)
	{
		Icon_Destination->SetVisibility(
			Display.bIsRoutePoint ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (Icon_Route)
	{
		Icon_Route->SetVisibility(
			Display.bIsRoutePoint ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	OnMarkerUpdated(Display.DistanceMeters, Display.bOffScreen, Display.ArrowAngleDeg);
}
