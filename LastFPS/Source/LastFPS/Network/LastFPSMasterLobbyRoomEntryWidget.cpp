#include "LastFPSMasterLobbyRoomEntryWidget.h"
#include "LastFPSMasterLobbyRoomEntry.h"
#include "LastFPSMasterLobbyPC.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void ULastFPSMasterLobbyRoomEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_JoinRoom)
	{
		Btn_JoinRoom->OnClicked.AddDynamic(this, &ULastFPSMasterLobbyRoomEntryWidget::OnJoinRoomClicked);
	}
}

void ULastFPSMasterLobbyRoomEntryWidget::NativeDestruct()
{
	if (Btn_JoinRoom)
	{
		Btn_JoinRoom->OnClicked.RemoveDynamic(this, &ULastFPSMasterLobbyRoomEntryWidget::OnJoinRoomClicked);
	}

	BoundEntry = nullptr;

	Super::NativeDestruct();
}

void ULastFPSMasterLobbyRoomEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	// ListView는 항목 위젯을 재사용하므로 매번 새 데이터로 다시 그린다.
	BoundEntry = Cast<ULastFPSMasterLobbyRoomEntry>(ListItemObject);
	RefreshDisplay();
	BP_OnRoomEntryUpdated(BoundEntry);
}

void ULastFPSMasterLobbyRoomEntryWidget::RefreshDisplay()
{
	if (!BoundEntry)
	{
		return;
	}

	const FMasterLobbyRoomInfo& Room = BoundEntry->RoomInfo;

	if (Txt_RoomName)
	{
		Txt_RoomName->SetText(FText::FromString(Room.RoomName));
	}

	if (Txt_HostName)
	{
		Txt_HostName->SetText(FText::FromString(Room.HostName));
	}

	if (Txt_PlayerCount)
	{
		Txt_PlayerCount->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Room.CurrentPlayers, Room.MaxPlayers)));
	}

	if (Btn_JoinRoom)
	{
		// 정원이 찬 방은 접속 요청 자체를 막는다.
		Btn_JoinRoom->SetIsEnabled(Room.CurrentPlayers < Room.MaxPlayers);
	}
}

void ULastFPSMasterLobbyRoomEntryWidget::OnJoinRoomClicked()
{
	if (!BoundEntry)
	{
		return;
	}

	if (ALastFPSMasterLobbyPC* LobbyPC = Cast<ALastFPSMasterLobbyPC>(GetOwningPlayer()))
	{
		LobbyPC->TravelToRoom(BoundEntry->RoomInfo);
	}
}
