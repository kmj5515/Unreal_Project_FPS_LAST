#include "UI/HUD/LastFPSWaveEnemyMarkerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

void ULastFPSWaveEnemyMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureNativeWidget();
}

void ULastFPSWaveEnemyMarkerWidget::EnsureNativeWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	ArrowText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ArrowText"));
	WidgetTree->RootWidget = ArrowText;

	ArrowText->SetText(FText::FromString(TEXT("▼")));
	ArrowText->SetJustification(ETextJustify::Center);
	ArrowText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.015f, 0.01f, 1.f)));
	ArrowText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	ArrowText->SetShadowOffset(FVector2D(1.5f, 1.5f));

	FSlateFontInfo Font = ArrowText->GetFont();
	Font.Size = 42;
	Font.OutlineSettings.OutlineSize = 2;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.12f, 0.f, 0.f, 1.f);
	ArrowText->SetFont(Font);
}
