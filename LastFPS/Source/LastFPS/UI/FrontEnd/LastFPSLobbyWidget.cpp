#include "UI/FrontEnd/LastFPSLobbyWidget.h"

#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Framework/LastFPSTravelEntryButton.h"
#include "UI/Framework/LastFPSUITags.h"

#include "Components/TextBlock.h"
#include "Game/Travel/LastFPSLevelTravelSubsystem.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSLobbyWidget, Log, All);

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

	const auto BindButton = [this](
		ULastFPSButtonBase* Button,
		void (ULastFPSLobbyWidget::*Handler)())
	{
		if (Button)
		{
			Button->OnClicked().AddUObject(this, Handler);
		}
	};

	BindButton(Button_Inventory, &ULastFPSLobbyWidget::HandleInventoryClicked);
	BindButton(Button_Consumable, &ULastFPSLobbyWidget::HandleConsumableClicked);
	BindButton(Button_Missions,&ULastFPSLobbyWidget::HandleMissionsClicked);
	BindButton(Button_Shop,&ULastFPSLobbyWidget::HandleShopClicked);
	BindButton(Button_Module,&ULastFPSLobbyWidget::HandleModuleClicked);
	BindButton(Button_Settings,&ULastFPSLobbyWidget::HandleSettingsClicked);
	BindButton(Button_BackToMain,&ULastFPSLobbyWidget::HandleBackToMainClicked);

	if (Button_GotoDungeon)
	{
		Button_GotoDungeon->OnClicked().AddUObject(this,&ULastFPSLobbyWidget::HandleGotoDungeonClicked);
	}
}

void ULastFPSLobbyWidget::HandleInventoryClicked()
{
	OpenScreenOrNotice(LastFPSUITags::Screen_Inventory(),NSLOCTEXT("LastFPS", "Lobby_Inventory", "인벤토리"));
}

void ULastFPSLobbyWidget::HandleConsumableClicked()
{
	OpenScreenOrNotice(LastFPSUITags::Screen_Consumable(),NSLOCTEXT("LastFPS", "Lobby_Consumable", "소모품"));
}

void ULastFPSLobbyWidget::HandleMissionsClicked()
{
	OpenScreenOrNotice(
		LastFPSUITags::Screen_Mission(),
		NSLOCTEXT("LastFPS", "Lobby_Missions", "임무"));
}

void ULastFPSLobbyWidget::HandleShopClicked()
{
	OpenScreenOrNotice(
		LastFPSUITags::Screen_Shop(),
		NSLOCTEXT("LastFPS", "Lobby_Shop", "상점"));
}

void ULastFPSLobbyWidget::HandleModuleClicked()
{
	OpenScreenOrNotice(
		LastFPSUITags::Screen_Module(),
		NSLOCTEXT("LastFPS", "Lobby_Module", "모듈"));
}

void ULastFPSLobbyWidget::HandleSettingsClicked()
{
	OpenScreenOrNotice(
		LastFPSUITags::Screen_Settings(),
		NSLOCTEXT("LastFPS", "Lobby_Settings", "설정"));
}

void ULastFPSLobbyWidget::HandleGotoDungeonClicked()
{
	APlayerController* PlayerController = GetOwningPlayer();
	ULastFPSGameInstance* GameInstance = GetGameInstance<ULastFPSGameInstance>();
	ULastFPSLevelTravelSubsystem* TravelSubsystem = GameInstance
		                                                ? GameInstance->GetSubsystem<ULastFPSLevelTravelSubsystem>()
		                                                : nullptr;
	if (!PlayerController || !TravelSubsystem || !Button_GotoDungeon)
	{
		UE_LOG(
			LogLastFPSLobbyWidget,
			Error,
			TEXT(
				"이동을 시작할 수 없습니다: Widget=%s, "
				"PlayerController=%s, TravelSubsystem=%s, Button=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PlayerController),
			*GetNameSafe(TravelSubsystem),
			*GetNameSafe(Button_GotoDungeon));
		return;
	}

	const FLastFPSTravelEntryRequest& Request = Button_GotoDungeon->GetTravelRequest();
	const ELastFPSTravelRequestResult Result = TravelSubsystem->RequestTravel(
		PlayerController,
		Request);
	if (Result != ELastFPSTravelRequestResult::Accepted)
	{
		UE_LOG(
			LogLastFPSLobbyWidget,
			Warning,
			TEXT(
				"이동 요청이 거부되었습니다: Widget=%s, "
				"Type=%s, BattleDefinition=%s, Result=%s"),
			*GetNameSafe(this),
			*StaticEnum<ELastFPSTravelEntryType>()->GetNameStringByValue(
				static_cast<int64>(Request.Type)),
			*Request.BattleDefinitionId.ToString(),
			*StaticEnum<ELastFPSTravelRequestResult>()->GetNameStringByValue(
				static_cast<int64>(Result)));
	}

	DeactivateWidget();
}

void ULastFPSLobbyWidget::HandleBackToMainClicked()
{
	if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
	{
		GI->RequestTravelToMainMenu();
	}
}

void ULastFPSLobbyWidget::OpenScreenOrNotice(
	const FGameplayTag& ScreenTag,
	const FText& FeatureName)
{
	ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>();
	if (!PC)
	{
		return;
	}

	// 레지스트리에 등록돼 있으면 그 화면을 연다.
	if (PC->OpenScreen(ScreenTag))
	{
		return;
	}

	// 아직 없으면 "준비 중" 공지로 폴백.
	PC->ShowNotice(
		FeatureName,
		FText::Format(NSLOCTEXT("LastFPS", "Lobby_WIP", "{0} 기능은 준비 중입니다."), FeatureName));
}
