#include "UI/LastFPSLobbyWidget.h"

#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"
#include "UI/LastFPSButtonBase.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void ULastFPSLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TB_PlayerName)
	{
		if (const APlayerState* PS = GetOwningPlayerState())
		{
			TB_PlayerName->SetText(FText::FromString(PS->GetPlayerName()));
		}
	}

	if (Button_Inventory)   Button_Inventory->OnClicked().AddUObject(this, &ULastFPSLobbyWidget::HandleInventoryClicked);
	if (Button_Missions)    Button_Missions->OnClicked().AddUObject(this, &ULastFPSLobbyWidget::HandleMissionsClicked);
	if (Button_Shop)        Button_Shop->OnClicked().AddUObject(this, &ULastFPSLobbyWidget::HandleShopClicked);
	if (Button_Settings)    Button_Settings->OnClicked().AddUObject(this, &ULastFPSLobbyWidget::HandleSettingsClicked);
	if (Button_BackToMain)  Button_BackToMain->OnClicked().AddUObject(this, &ULastFPSLobbyWidget::HandleBackToMainClicked);
}

void ULastFPSLobbyWidget::HandleInventoryClicked()   { ShowWIPNotice(NSLOCTEXT("LastFPS", "Lobby_Inventory", "인벤토리")); }
void ULastFPSLobbyWidget::HandleMissionsClicked()    { ShowWIPNotice(NSLOCTEXT("LastFPS", "Lobby_Missions", "임무")); }
void ULastFPSLobbyWidget::HandleShopClicked()        { ShowWIPNotice(NSLOCTEXT("LastFPS", "Lobby_Shop", "상점")); }
void ULastFPSLobbyWidget::HandleSettingsClicked()    { ShowWIPNotice(NSLOCTEXT("LastFPS", "Lobby_Settings", "설정")); }

void ULastFPSLobbyWidget::HandleBackToMainClicked()
{
	if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
	{
		GI->RequestTravelToMainMenu();
	}
}

void ULastFPSLobbyWidget::ShowWIPNotice(const FText& FeatureName)
{
	if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
	{
		PC->ShowNotice(
			FeatureName,
			FText::Format(NSLOCTEXT("LastFPS", "Lobby_WIP", "{0} 기능은 준비 중입니다."), FeatureName));
	}
}
