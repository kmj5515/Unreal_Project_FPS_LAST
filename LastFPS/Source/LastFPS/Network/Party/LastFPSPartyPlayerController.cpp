#include "LastFPSPartyPlayerController.h"
#include "LastFPSPartyGameMode.h"
#include "LastFPSPartyPlayerState.h"
#include "UI/Framework/LastFPSPrimaryGameLayout.h"
#include "PrimaryGameLayout.h"
#include "UI/Framework/LastFPSUITags.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/UserWidget.h"

ALastFPSPartyPlayerController::ALastFPSPartyPlayerController()
{
	PartyPanelWidgetClass = TSoftClassPtr<UCommonActivatableWidget>(FSoftObjectPath(TEXT("/Game/UI/FrontEnd/WBP_PartyPanel.WBP_PartyPanel_C")));
}

void ALastFPSPartyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		SetShowMouseCursor(true);
		SetInputMode(FInputModeUIOnly());
	}
}

void ALastFPSPartyPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalPlayerController())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PartyPlayerController] ReceivedPlayer called! Pushing UI..."));
		
		TSoftClassPtr<UCommonActivatableWidget> PartyWidgetPtr(FSoftClassPath(TEXT("/Game/UI/FrontEnd/WBP_PartyPanel.WBP_PartyPanel_C")));

		FTimerHandle RetryTimer;
		GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this, PartyWidgetPtr]()
		{
			if (ULastFPSPrimaryGameLayout* RootLayout = Cast<ULastFPSPrimaryGameLayout>(UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this)))
			{
				if (UClass* WidgetClass = PartyWidgetPtr.LoadSynchronous())
				{
					RootLayout->PushWidgetToLayerStack<UCommonActivatableWidget>(LastFPSUITags::Layer_Menu(), WidgetClass);
					
					// Force add to viewport in case Layer_Menu is hidden
					if (UCommonActivatableWidget* ForceWidget = CreateWidget<UCommonActivatableWidget>(this, WidgetClass))
					{
						ForceWidget->AddToViewport(9999);
						ForceWidget->ActivateWidget();
					}

					UE_LOG(LogTemp, Warning, TEXT("[PartyPlayerController] Successfully pushed PartyPanel via RootLayout (Synchronous)!"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[PartyPlayerController] RootLayout is NULL! Falling back to AddToViewport."));
				if (UClass* WidgetClass = PartyWidgetPtr.LoadSynchronous())
				{
					if (UCommonActivatableWidget* PartyWidget = CreateWidget<UCommonActivatableWidget>(this, WidgetClass))
					{
						PartyWidget->AddToViewport();
						PartyWidget->ActivateWidget();
						UE_LOG(LogTemp, Warning, TEXT("[PartyPlayerController] Successfully added PartyPanel directly to viewport!"));
					}
				}
			}
		}, 0.5f, false);
	}
}

void ALastFPSPartyPlayerController::ServerSetPartyReady_Implementation(bool bIsReady)
{
	if (ALastFPSPartyPlayerState* PS = GetPlayerState<ALastFPSPartyPlayerState>())
	{
		PS->SetPartyReady(bIsReady);
	}
}

void ALastFPSPartyPlayerController::ServerRequestStartPartyGame_Implementation()
{
	if (ALastFPSPartyGameMode* GM = GetWorld()->GetAuthGameMode<ALastFPSPartyGameMode>())
	{
		GM->RequestStartGame(this);
	}
}
