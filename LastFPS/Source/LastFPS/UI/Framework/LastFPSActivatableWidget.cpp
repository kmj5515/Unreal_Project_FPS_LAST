#include "UI/Framework/LastFPSActivatableWidget.h"

#include "UI/Framework/LastFPSPrimaryGameLayout.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIInputTypes.h"
#include "Kismet/GameplayStatics.h"
#include "PrimaryGameLayout.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

ULastFPSActivatableWidget::ULastFPSActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// CommonUI Back 라우팅은 입력 config 적용 경로를 함께 쓰는데, 이 프로젝트는
	// bEnableDefaultInputConfig=False 라 그 경로가 돌지 않는다. back-handler 로 등록해도
	// 호출되지 않으므로 끄고, 닫기는 아래 raw ESC 로 처리한다.
	bIsBackHandler = false;
}

TOptional<FUIInputConfig> ULastFPSActivatableWidget::GetDesiredInputConfig() const
{
	// 메뉴/모달은 항상 커서가 보이는 Menu 모드. 스택이 바뀔 때마다(모달 pop 포함)
	// CommonUI가 이 config를 재적용하므로 커서가 사라지지 않는다.
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

bool ULastFPSActivatableWidget::NativeOnHandleBackAction()
{
	// Back/ESC 단일 종료 지점. 모달은 오버라이드해 결과 브로드캐스트 후 닫음.
	DeactivateWidget();
	return true;
}

void ULastFPSActivatableWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// BP 직렬화 값이 생성자 설정을 덮을 수 있어 여기서 재차 강제.
	bIsBackHandler = false;
}

void ULastFPSActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (ActivateSound)
	{
		UGameplayStatics::PlaySound2D(this, ActivateSound);
	}

	// 키 입력(ESC)을 받으려면 포커스가 필요. CommonUI Back 이 미구성이라 직접 확보.
	SetIsFocusable(true);
	SetKeyboardFocus();
}

void ULastFPSActivatableWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	if (DeactivateSound)
	{
		UGameplayStatics::PlaySound2D(this, DeactivateSound);
	}

	// 스택 제거 완료 후(다음 틱) 아래 레이어 최상위 위젯으로 포커스 복원.
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
	if (bCloseOnEscape && InKeyEvent.GetKey() == EKeys::Escape)
	{
		// raw ESC → 단일 종료 지점(NativeOnHandleBackAction)으로 위임.
		if (NativeOnHandleBackAction())
		{
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

