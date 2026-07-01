#include "UI/Hub/LastFPSNPCInteractionWidget.h"

#include "Game/LastFPSPlayerController.h"
#include "UI/Framework/LastFPSButtonBase.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Input/CommonUIInputTypes.h"

void ULastFPSNPCInteractionWidget::Setup(
	ALastFPSPlayerController* InPC,
	const FText& InName,
	const FText& InRole,
	const TArray<FLastFPSNPCAction>& InActions)
{
	OwningPC = InPC;

	if (TB_Name)
	{
		TB_Name->SetText(InName);
	}
	if (TB_Role)
	{
		TB_Role->SetText(InRole);
		TB_Role->SetVisibility(InRole.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	if (!ButtonContainer)
	{
		return;
	}

	ButtonContainer->ClearChildren();

	if (!ActionButtonClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("LastFPSNPCInteractionWidget: ActionButtonClass 미지정 → 버튼을 만들 수 없습니다. WBP_NPCInteraction에서 지정하세요."));
		return;
	}

	for (const FLastFPSNPCAction& Action : InActions)
	{
		ULastFPSButtonBase* Button = CreateWidget<ULastFPSButtonBase>(this, ActionButtonClass);
		if (!Button)
		{
			continue;
		}

		Button->SetButtonText(Action.Label);
		ButtonContainer->AddChild(Button);

		// 클릭 시 해당 액션 실행. Action을 값으로 캡처해 각 버튼이 자기 동작을 기억.
		Button->OnClicked().AddWeakLambda(this, [this, Action]()
		{
			if (ALastFPSPlayerController* PC = OwningPC.Get())
			{
				PC->ExecuteNPCAction(Action);
			}
		});
	}
}

void ULastFPSNPCInteractionWidget::NativeDestruct()
{
	// 허브가 닫히면(ESC/Back으로 스택에서 pop) 캐릭터 카메라·이동으로 복귀.
	if (ALastFPSPlayerController* PC = OwningPC.Get())
	{
		PC->EndNPCInteraction();
	}

	Super::NativeDestruct();
}

TOptional<FUIInputConfig> ULastFPSNPCInteractionWidget::GetDesiredInputConfig() const
{
	// Menu: 게임 입력 억제(이동 정지) + 마우스 커서 표시로 버튼 클릭 가능.
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
