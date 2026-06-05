#include "UI/LastFPSMainMenuWidget.h"

#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"
#include "UI/LastFPSButtonBase.h"

#include "Kismet/KismetSystemLibrary.h"

void ULastFPSMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Start)
	{
		Button_Start->OnClicked().AddUObject(this, &ULastFPSMainMenuWidget::HandleStartClicked);
	}
	if (Button_Settings)
	{
		Button_Settings->OnClicked().AddUObject(this, &ULastFPSMainMenuWidget::HandleSettingsClicked);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked().AddUObject(this, &ULastFPSMainMenuWidget::HandleQuitClicked);
	}
}

void ULastFPSMainMenuWidget::HandleStartClicked()
{
	if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
	{
		// [임시] 캐릭터 선택 미구현 — Hub 직행
		GI->RequestTravelToHub();
	}
}

void ULastFPSMainMenuWidget::HandleSettingsClicked()
{
	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		PC->ShowNotice(
			NSLOCTEXT("LastFPS", "SettingsNoticeTitle", "설정"),
			NSLOCTEXT("LastFPS", "SettingsNoticeBody", "설정 화면은 준비 중입니다."));
	}
}

void ULastFPSMainMenuWidget::HandleQuitClicked()
{
	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		FLastFPSConfirmResultDelegate ResultDelegate;
		ResultDelegate.BindUFunction(this, FName("HandleQuitConfirmResult"));
		PC->ShowConfirm(
			NSLOCTEXT("LastFPS", "QuitConfirmTitle", "게임 종료"),
			NSLOCTEXT("LastFPS", "QuitConfirmBody", "정말 종료하시겠습니까?"),
			ResultDelegate);
	}
}

void ULastFPSMainMenuWidget::HandleQuitConfirmResult(const bool bConfirmed)
{
	if (!bConfirmed)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
}
