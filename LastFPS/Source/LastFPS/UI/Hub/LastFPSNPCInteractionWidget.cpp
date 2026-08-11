#include "UI/Hub/LastFPSNPCInteractionWidget.h"

#include "Game/LastFPSPlayerController.h"
#include "UI/Framework/LastFPSButtonBase.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Input/CommonUIInputTypes.h"

namespace
{
	// 컨테이너가 VerticalBox/HorizontalBox 어느 쪽이든 슬롯 여백을 적용.
	void ApplyButtonSlotPadding(UPanelSlot* Slot, const FMargin& Padding)
	{
		if (UVerticalBoxSlot* VBSlot = Cast<UVerticalBoxSlot>(Slot))
		{
			VBSlot->SetPadding(Padding);
		}
		else if (UHorizontalBoxSlot* HBSlot = Cast<UHorizontalBoxSlot>(Slot))
		{
			HBSlot->SetPadding(Padding);
		}
	}
}

void ULastFPSNPCInteractionWidget::Setup(
	ALastFPSPlayerController* InPC,
	const FText& InName,
	const FText& InDescription,
	const TArray<FLastFPSNPCAction>& InActions,
	const TArray<FLastFPSNPCQuestOption>& InQuestOptions)
{
	OwningPC = InPC;
	InitialFocusTarget.Reset();

	if (TB_Name)
	{
		TB_Name->SetText(InName);
	}
	if (TB_Role)
	{
		TB_Role->SetText(InDescription);
		TB_Role->SetVisibility(InDescription.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
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

	// 레퍼런스의 하단 내용 행 자체가 수락/보고 입력이다. 별도의 확인 버튼을 만들지 않는다.
	for (const FLastFPSNPCQuestOption& QuestOption : InQuestOptions)
	{
		ULastFPSButtonBase* Button = CreateWidget<ULastFPSButtonBase>(GetOwningPlayer(), ActionButtonClass);
		if (!Button)
		{
			continue;
		}

		Button->SetButtonText(QuestOption.Content.IsEmpty() ? QuestOption.Title : QuestOption.Content);
		if (!QuestOption.Description.IsEmpty())
		{
			Button->SetToolTipText(QuestOption.Description);
		}
		ApplyButtonSlotPadding(ButtonContainer->AddChild(Button), ButtonPadding);
		if (!InitialFocusTarget.IsValid())
		{
			InitialFocusTarget = Button;
		}

		Button->OnClicked().AddWeakLambda(this, [this, QuestOption]()
		{
			if (ALastFPSPlayerController* PC = OwningPC.Get())
			{
				if (PC->ExecuteNPCQuestOption(QuestOption))
				{
					DeactivateWidget();
				}
			}
		});
	}

	for (const FLastFPSNPCAction& Action : InActions)
	{
		ULastFPSButtonBase* Button = CreateWidget<ULastFPSButtonBase>(GetOwningPlayer(), ActionButtonClass);
		if (!Button)
		{
			continue;
		}

		Button->SetButtonText(Action.Label);
		ApplyButtonSlotPadding(ButtonContainer->AddChild(Button), ButtonPadding);
		if (!InitialFocusTarget.IsValid())
		{
			InitialFocusTarget = Button;
		}

		// 클릭 시 해당 액션 실행. Action을 값으로 캡처해 각 버튼이 자기 동작을 기억.
		Button->OnClicked().AddWeakLambda(this, [this, Action]()
		{
			if (ALastFPSPlayerController* PC = OwningPC.Get())
			{
				PC->ExecuteNPCAction(Action);
			}
		});
	}

	// 기본 "나가기" 버튼 — 항상 맨 아래에 자동 추가. 클릭 시 허브를 닫아 상호작용 종료(=ESC와 동일).
	if (bShowExitButton)
	{
		if (ULastFPSButtonBase* ExitButton = CreateWidget<ULastFPSButtonBase>(GetOwningPlayer(), ActionButtonClass))
		{
			ExitButton->SetButtonText(ExitButtonLabel);
			ApplyButtonSlotPadding(ButtonContainer->AddChild(ExitButton), ButtonPadding);
			if (!InitialFocusTarget.IsValid())
			{
				InitialFocusTarget = ExitButton;
			}
			ExitButton->OnClicked().AddWeakLambda(this, [this]()
			{
				DeactivateWidget(); // 스택에서 pop → NativeDestruct → EndNPCInteraction
			});
		}
	}

	RequestRefreshFocus();
}

void ULastFPSNPCInteractionWidget::SetButtonsVisible(bool bVisible)
{
	if (ButtonContainer)
	{
		ButtonContainer->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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

UWidget* ULastFPSNPCInteractionWidget::NativeGetDesiredFocusTarget() const
{
	return InitialFocusTarget.IsValid()
		? InitialFocusTarget.Get()
		: Super::NativeGetDesiredFocusTarget();
}

TOptional<FUIInputConfig> ULastFPSNPCInteractionWidget::GetDesiredInputConfig() const
{
	// Menu: 게임 입력 억제(이동 정지) + 마우스 커서 표시로 버튼 클릭 가능.
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
