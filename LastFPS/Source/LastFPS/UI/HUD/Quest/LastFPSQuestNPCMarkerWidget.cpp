#include "UI/HUD/Quest/LastFPSQuestNPCMarkerWidget.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULastFPSQuestNPCMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Img_Symbol)
	{
		Img_Symbol->SetColorAndOpacity(SymbolColor);
	}
}

void ULastFPSQuestNPCMarkerWidget::ApplyMarkerState(
	const ELastFPSQuestNPCMarkerSymbol Symbol,
	const bool bTracked)
{
	if (Img_Symbol)
	{
		// None 은 컴포넌트가 마커 자체를 숨기므로 여기서는 브러시를 바꾸지 않는다.
		if (Symbol == ELastFPSQuestNPCMarkerSymbol::Available)
		{
			Img_Symbol->SetBrush(AvailableSymbolBrush);
		}
		else if (Symbol == ELastFPSQuestNPCMarkerSymbol::Objective)
		{
			Img_Symbol->SetBrush(ObjectiveSymbolBrush);
		}
		else if (Symbol == ELastFPSQuestNPCMarkerSymbol::InProgress)
		{
			Img_Symbol->SetBrush(InProgressSymbolBrush);
		}

		Img_Symbol->SetColorAndOpacity(SymbolColor);
	}

	const FLinearColor ColorTop = bTracked ? TrackedColorTop : IdleColorTop;
	const FLinearColor ColorBottom = bTracked ? TrackedColorBottom : IdleColorBottom;

	// 첫 상태 반영이 위젯 생성보다 먼저 올 수 있어 동적 인스턴스는 여기서 지연 생성한다.
	// (브러시가 텍스처면 null 이 돌아오고 아래 단색 폴백을 탄다.)
	if (!BackgroundMID && Img_Background)
	{
		BackgroundMID = Img_Background->GetDynamicMaterial();
	}

	if (BackgroundMID)
	{
		BackgroundMID->SetVectorParameterValue(ColorTopParam, ColorTop);
		BackgroundMID->SetVectorParameterValue(ColorBottomParam, ColorBottom);
	}
	else if (Img_Background)
	{
		// 머티리얼이 없으면 그라디언트를 표현할 수 없으므로 위쪽 색으로 단색 처리한다.
		Img_Background->SetColorAndOpacity(ColorTop);
	}

	OnMarkerStateChanged(Symbol, bTracked);
}
