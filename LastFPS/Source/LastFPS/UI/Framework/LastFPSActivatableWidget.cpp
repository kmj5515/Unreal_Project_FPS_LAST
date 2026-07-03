#include "UI/Framework/LastFPSActivatableWidget.h"

#include "UI/Framework/LastFPSPrimaryGameLayout.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIInputTypes.h"
#include "PrimaryGameLayout.h"
#include "TimerManager.h"

ULastFPSActivatableWidget::ULastFPSActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

TOptional<FUIInputConfig> ULastFPSActivatableWidget::GetDesiredInputConfig() const
{
	// 메뉴/모달은 항상 커서가 보이는 Menu 모드. 스택이 바뀔 때마다(모달 pop 포함)
	// CommonUI가 이 config를 재적용하므로 커서가 사라지지 않는다.
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

bool ULastFPSActivatableWidget::NativeOnHandleBackAction()
{
	return Super::NativeOnHandleBackAction();
}

void ULastFPSActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 키 입력(ESC)을 받으려면 포커스가 필요. CommonUI Back이 미구성이라 직접 확보.
	SetIsFocusable(true);
	SetKeyboardFocus();
}

void ULastFPSActivatableWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	// 스택에서 제거가 끝난 뒤 조회하도록 다음 틱으로 미룬 후, 아래 레이어 최상위 위젯으로 포커스 복원.
	// 타이머는 (파괴될 수 있는) 위젯이 아니라 영속적인 PlayerController/Layout 기준으로 안전하게 실행.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TWeakObjectPtr<APlayerController> WeakPC = GetOwningPlayer();
	World->GetTimerManager().SetTimerForNextTick([WeakPC]()
	{
		APlayerController* PC = WeakPC.Get();
		if (!PC)
		{
			return;
		}
		if (ULastFPSPrimaryGameLayout* Layout =
				Cast<ULastFPSPrimaryGameLayout>(UPrimaryGameLayout::GetPrimaryGameLayout(PC)))
		{
			Layout->RestoreFocusToTopActiveWidget();
		}
	});
}

FReply ULastFPSActivatableWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bIsBackHandler && InKeyEvent.GetKey() == EKeys::Escape)
	{
		DeactivateWidget();   // 레이어 스택에서 pop → 입력모드 복귀
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
