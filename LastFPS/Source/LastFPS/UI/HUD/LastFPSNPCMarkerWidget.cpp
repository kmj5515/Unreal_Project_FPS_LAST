#include "UI/HUD/LastFPSNPCMarkerWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULastFPSNPCMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 게이지 이미지 브러시에 머티리얼(MI_Progress_Radial)이 지정돼 있으면
	// 동적 인스턴스를 만들어 두고, 매 프레임 진행도를 그 스칼라 파라미터에 쓴다.
	if (Img_HoldGauge)
	{
		GaugeMID = Img_HoldGauge->GetDynamicMaterial();
	}
}

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

	// 원형 게이지 머티리얼 구동 (MI_Progress_Radial 의 "Progress" 스칼라).
	// 이 머티리얼은 값↑일수록 흰색이 비워지므로, "채워지게" 보이려면 반전해서 넣는다.
	if (GaugeMID)
	{
		const float GaugeValue = bInvertGaugeFill ? (1.f - Progress) : Progress;
		GaugeMID->SetScalarParameterValue(GaugeProgressParam, GaugeValue);
	}

	// 추가 연출이 필요할 때만 BP 측에서 사용 (없으면 무동작).
	OnInteractionProgressChanged(Progress);
}
