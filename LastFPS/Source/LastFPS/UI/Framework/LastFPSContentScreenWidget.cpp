#include "UI/Framework/LastFPSContentScreenWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Theme/LastFPSUITheme.h"

void ULastFPSContentScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 자신부터 시작해야 화면 위젯 자체도 테마를 받는다(하위 트리만 돌면 화면은 빠진다).
	LastFPSUITheme::ApplyToWidgetTree(this);

	if (TB_Title)
	{
		TB_Title->SetText(ScreenTitle);
	}

	if (Button_Close)
	{
		Button_Close->OnClicked().AddUObject(this, &ULastFPSContentScreenWidget::HandleCloseClicked);
	}

	// 껍데기가 SetHostedInShell 을 먼저 부를 수도, 나중에 부를 수도 있어 양쪽에서 적용한다.
	ApplyChromeVisibility();
}

void ULastFPSContentScreenWidget::SetHostedInShell(const bool bInHostedInShell)
{
	if (bHostedInShell == bInHostedInShell)
	{
		return;
	}

	bHostedInShell = bInHostedInShell;
	if (bHostedInShell)
	{
		// 셸 내부 콘텐츠가 ESC 를 먼저 소비하면 자신만 비활성화되고 바깥 메뉴가 남는다.
		// 스위처의 자식이라 DeactivateWidget 이 pop 도 아니어서 아무 일이 일어나지 않는다.
		// 호스트된 콘텐츠는 ESC 를 처리하지 않고 상위 HubMenu 까지 전달한다.
		bCloseOnEscape = false;
	}

	ApplyChromeVisibility();
}

void ULastFPSContentScreenWidget::ApplyChromeVisibility()
{
	const ESlateVisibility ChromeVisibility =
		bHostedInShell ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;

	// 상단바 컨테이너가 바인딩돼 있으면 통째로 접는 것이 가장 깔끔하다.
	if (Panel_ScreenChrome)
	{
		Panel_ScreenChrome->SetVisibility(ChromeVisibility);
		return;
	}

	// 컨테이너 이름을 맞추지 않은 화면은 최소한 중복되는 요소만이라도 지운다.
	if (TB_Title)
	{
		TB_Title->SetVisibility(ChromeVisibility);
	}
	if (Button_Close)
	{
		Button_Close->SetVisibility(ChromeVisibility);
	}
}

void ULastFPSContentScreenWidget::HandleCloseClicked()
{
	// 레이어 스택에서 자신을 pop. Back 액션과 동일 결과.
	DeactivateWidget();
}
