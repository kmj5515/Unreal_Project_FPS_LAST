#include "UI/HUD/LastFPSWaveEnemyMarkerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

void ULastFPSWaveEnemyMarkerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Slate 루트가 만들어지기 전에 WidgetTree를 구성해야 네이티브 위젯이 실제 화면에 포함된다.
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

	// 소스 파일 인코딩과 무관하게 항상 동일한 아래쪽 삼각형 문자를 생성한다.
	ArrowText->SetText(FText::FromString(FString::Chr(0x25BC)));
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
