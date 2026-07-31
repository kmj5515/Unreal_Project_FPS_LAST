#include "UI/Common/LastFPSQuantityDialogWidget.h"

#include "UI/Framework/LastFPSButtonBase.h"
#include "Components/TextBlock.h"

void ULastFPSQuantityDialogWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 인스턴스 최초 1회만 바인딩 (재오픈 시 누적 방지)
	if (Button_Confirm) Button_Confirm->OnClicked().AddUObject(this, &ULastFPSQuantityDialogWidget::HandleConfirmClicked);
	if (Button_Cancel)  Button_Cancel->OnClicked().AddUObject(this,  &ULastFPSQuantityDialogWidget::HandleCancelClicked);
	if (Button_Plus)    Button_Plus->OnClicked().AddUObject(this,    &ULastFPSQuantityDialogWidget::HandlePlusClicked);
	if (Button_Minus)   Button_Minus->OnClicked().AddUObject(this,   &ULastFPSQuantityDialogWidget::HandleMinusClicked);
}

bool ULastFPSQuantityDialogWidget::NativeOnHandleBackAction()
{
	HandleCancelClicked();
	return true;
}

void ULastFPSQuantityDialogWidget::KillDialog()
{
	CloseWithResult(0);
}

void ULastFPSQuantityDialogWidget::SetupQuantity(const FText& InTitle, const FText& InItemName, int32 InUnitPrice, int32 InMaxQuantity)
{
	UnitPrice       = FMath::Max(0, InUnitPrice);
	MaxQuantity     = FMath::Max(1, InMaxQuantity);
	CurrentQuantity = 1;

	// 재사용(풀) 인스턴스라 이전 호출의 결과 구독이 남아 있을 수 있어 매번 초기화.
	// (호출부 ShowQuantityPrompt 가 이 직후 새 OnResult 를 1개만 다시 Add 한다.)
	OnQuantityResult.Clear();

	SetDialogText(InTitle, InItemName);

	if (TB_MaxInfo)
	{
		TB_MaxInfo->SetText(FText::FromString(FString::Printf(TEXT("최대 %d개 구매 가능"), MaxQuantity)));
	}

	RefreshDisplay();
}

void ULastFPSQuantityDialogWidget::RefreshDisplay()
{
	if (TB_Quantity)   { TB_Quantity->SetText(FText::AsNumber(CurrentQuantity)); }
	if (TB_TotalPrice) { TB_TotalPrice->SetText(FText::AsNumber(UnitPrice * CurrentQuantity)); }

	// 한도에 닿으면 해당 방향 버튼 비활성
	if (Button_Plus)  { Button_Plus->SetIsEnabled(CurrentQuantity < MaxQuantity); }
	if (Button_Minus) { Button_Minus->SetIsEnabled(CurrentQuantity > 1); }
}

void ULastFPSQuantityDialogWidget::HandlePlusClicked()
{
	CurrentQuantity = FMath::Min(CurrentQuantity + 1, MaxQuantity);
	RefreshDisplay();
}

void ULastFPSQuantityDialogWidget::HandleMinusClicked()
{
	CurrentQuantity = FMath::Max(CurrentQuantity - 1, 1);
	RefreshDisplay();
}

void ULastFPSQuantityDialogWidget::HandleConfirmClicked() { CloseWithResult(CurrentQuantity); }
void ULastFPSQuantityDialogWidget::HandleCancelClicked()  { CloseWithResult(0); }

void ULastFPSQuantityDialogWidget::CloseWithResult(const int32 Quantity)
{
	OnQuantityResult.Broadcast(Quantity);
	DeactivateWithAnimation();
}
