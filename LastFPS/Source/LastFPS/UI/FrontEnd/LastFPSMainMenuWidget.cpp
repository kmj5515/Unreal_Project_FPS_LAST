#include "UI/FrontEnd/LastFPSMainMenuWidget.h"

#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"
#include "Localization/LastFPSLocalization.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Framework/LastFPSUITags.h"

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
		GI->RequestTravelToCharacterSelect();
	}
}

void ULastFPSMainMenuWidget::HandleSettingsClicked()
{
	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		// 등록 시 설정 화면 오픈, 미등록이면 OpenScreen이 nullptr 반환 → 준비 중 공지 폴백.
		if (!PC->OpenScreen(LastFPSUITags::Screen_Settings()))
		{
			PC->ShowNotice(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::SettingsNoticeTitle),
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::SettingsNoticeBody));
		}
	}
}

void ULastFPSMainMenuWidget::HandleQuitClicked()
{
	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		FLastFPSConfirmResultDelegate ResultDelegate;
		ResultDelegate.BindUFunction(this, FName("HandleQuitConfirmResult"));
		PC->ShowConfirm(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuitGameTitle),
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuitGameMainMenuConfirmation),
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
