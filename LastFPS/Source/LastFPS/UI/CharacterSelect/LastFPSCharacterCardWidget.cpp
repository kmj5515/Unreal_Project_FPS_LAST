#include "UI/CharacterSelect/LastFPSCharacterCardWidget.h"

#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Input/Events.h"

void ULastFPSCharacterCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 카드 전체가 마우스 클릭을 받도록 가시성을 코드에서 강제(에디터 설정 의존 제거).
	SetVisibility(ESlateVisibility::Visible);

	RefreshFrameVisual(/*bHovered=*/false);
}

void ULastFPSCharacterCardWidget::SetupCard_Implementation(const ULastFPSCharacterDefinition* Def)
{
	if (TB_CardName)
	{
		TB_CardName->SetText(Def ? Def->DisplayName : FText::GetEmpty());
	}
	if (TB_CardRole)
	{
		TB_CardRole->SetText(Def ? Def->Role : FText::GetEmpty());
	}
}

void ULastFPSCharacterCardWidget::SetSelected_Implementation(bool bSelected)
{
	bIsSelected = bSelected;
	SetRenderScale(FVector2D(bSelected ? SelectedScale : 1.0f));
	RefreshFrameVisual(/*bHovered=*/false);
}

void ULastFPSCharacterCardWidget::RefreshFrameVisual(bool bHovered)
{
	if (!OuterFrame)
	{
		return;
	}

	// 우선순위: 선택 > 호버 > 평상시
	const FLinearColor Color =
		bIsSelected ? SelectedFrameColor :
		bHovered    ? HoverFrameColor    :
		              NormalFrameColor;

	OuterFrame->SetBrushColor(Color);
}

FReply ULastFPSCharacterCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCardClicked.ExecuteIfBound(CardIndex);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULastFPSCharacterCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	RefreshFrameVisual(/*bHovered=*/true);
}

void ULastFPSCharacterCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	RefreshFrameVisual(/*bHovered=*/false);
}
