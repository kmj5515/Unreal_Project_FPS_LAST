#include "UI/HUD/LastFPSCaptureObjectiveWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Localization/LastFPSLocalization.h"

void ULastFPSCaptureObjectiveWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Bar_Capture)
	{
		Bar_Capture->SetFillColorAndOpacity(CaptureFillColor);
	}
}

void ULastFPSCaptureObjectiveWidget::SetupObjective(const FText& InLabel)
{
	if (Text_Label)
	{
		Text_Label->SetText(InLabel);
	}

	LastProgress = 0.f;
	bStalled = false;
	LastAdvanceTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	UpdateProgress(0.f);
}

void ULastFPSCaptureObjectiveWidget::UpdateProgress(const float Progress01)
{
	const float Clamped = FMath::Clamp(Progress01, 0.f, 1.f);
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	if (Clamped > LastProgress + KINDA_SMALL_NUMBER)
	{
		LastAdvanceTime = Now;
		ApplyStalled(false);
	}
	else if (Now - LastAdvanceTime >= StallDetectDelay)
	{
		// 갱신 주기 사이의 정상적인 정지와 구분하기 위해 유예를 둔다.
		ApplyStalled(true);
	}
	LastProgress = Clamped;

	if (Bar_Capture)
	{
		Bar_Capture->SetPercent(Clamped);
	}

	if (Text_Percent)
	{
		Text_Percent->SetText(FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::PercentFormat),
			FText::AsNumber(FMath::FloorToInt(Clamped * 100.f))));
	}

	OnCaptureDisplayUpdated(Clamped, bStalled);
}

void ULastFPSCaptureObjectiveWidget::ApplyStalled(const bool bNewStalled)
{
	if (bStalled == bNewStalled)
	{
		return;
	}
	bStalled = bNewStalled;

	if (Bar_Capture)
	{
		Bar_Capture->SetFillColorAndOpacity(bStalled ? StalledFillColor : CaptureFillColor);
	}
}
