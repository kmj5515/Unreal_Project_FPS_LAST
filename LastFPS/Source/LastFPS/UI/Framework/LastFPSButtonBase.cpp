#include "UI/Framework/LastFPSButtonBase.h"
#include "Components/SizeBox.h"
#include "CommonUI/Public/CommonTextBlock.h"
#include "Sound/SoundBase.h"

void ULastFPSButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (TextBlock)
	{
		TextBlock->SetText(ButtonText);
	}
}

void ULastFPSButtonBase::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();

	// 바인딩 타입을 UCommonTextBlock 으로 좁히면, 라벨이 순수 TextBlock 인 기존 WBP 는 바인딩이
	// 조용히 끊겨 글자가 사라진다. 그래서 타입은 그대로 두고 여기서 확인만 한다.
	// 결과적으로 라벨을 CommonTextBlock 으로 저작한 WBP 만 상태별 스타일을 받는다.
	if (UCommonTextBlock* StyledText = Cast<UCommonTextBlock>(TextBlock))
	{
		StyledText->SetStyle(GetCurrentTextStyleClass());
	}
}

void ULastFPSButtonBase::SetButtonText(const FText& InText)
{
	ButtonText = InText;

	if (TextBlock)
	{
		TextBlock->SetText(InText);
	}
}
