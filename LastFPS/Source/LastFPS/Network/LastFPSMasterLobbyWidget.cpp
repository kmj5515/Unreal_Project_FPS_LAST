#include "LastFPSMasterLobbyWidget.h"
#include "LastFPSMasterLobbyClientSubsystem.h"
#include "LastFPSMasterLobbyPC.h"
#include "LastFPSMasterLobbyRoomEntry.h"
#include "Engine/GameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"

void ULastFPSMasterLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_CreateRoom)
	{
		Btn_CreateRoom->OnClicked.AddDynamic(this, &ULastFPSMasterLobbyWidget::OnCreateRoomClicked);
	}

	if (Btn_RefreshRooms)
	{
		Btn_RefreshRooms->OnClicked.AddDynamic(this, &ULastFPSMasterLobbyWidget::OnRefreshRoomsClicked);
	}

	if (ALastFPSMasterLobbyPC* LobbyPC = GetLobbyPlayerController())
	{
		BoundLobbyPC = LobbyPC;
		RoomListUpdatedHandle = LobbyPC->OnRoomListUpdatedNative.AddUObject(this, &ULastFPSMasterLobbyWidget::HandleRoomListUpdated);

		// 위젯이 첫 수신보다 늦게 생성될 수 있으므로 캐시된 목록으로 즉시 채운다.
		HandleRoomListUpdated(LobbyPC->GetCachedRoomList());
	}
	else
	{
		UE_LOG(LogMasterLobby, Warning, TEXT("마스터 로비 위젯이 ALastFPSMasterLobbyPC를 찾지 못해 방 목록을 구독할 수 없다."));
	}
}

void ULastFPSMasterLobbyWidget::NativeDestruct()
{
	if (Btn_CreateRoom)
	{
		Btn_CreateRoom->OnClicked.RemoveDynamic(this, &ULastFPSMasterLobbyWidget::OnCreateRoomClicked);
	}

	if (Btn_RefreshRooms)
	{
		Btn_RefreshRooms->OnClicked.RemoveDynamic(this, &ULastFPSMasterLobbyWidget::OnRefreshRoomsClicked);
	}

	// 컨트롤러가 살아 있을 때만 해제한다. 맵 이동으로 이미 파괴됐으면 무시한다.
	if (RoomListUpdatedHandle.IsValid())
	{
		if (ALastFPSMasterLobbyPC* LobbyPC = BoundLobbyPC.Get())
		{
			LobbyPC->OnRoomListUpdatedNative.Remove(RoomListUpdatedHandle);
		}
		RoomListUpdatedHandle.Reset();
	}
	BoundLobbyPC.Reset();

	Super::NativeDestruct();
}

ALastFPSMasterLobbyPC* ULastFPSMasterLobbyWidget::GetLobbyPlayerController() const
{
	return Cast<ALastFPSMasterLobbyPC>(GetOwningPlayer());
}

void ULastFPSMasterLobbyWidget::HandleRoomListUpdated(const TArray<FMasterLobbyRoomInfo>& RoomList)
{
	if (!ListView_Rooms)
	{
		return;
	}

	// 방 개수가 많지 않고 갱신 빈도도 낮아 전체 재구성이 부분 갱신보다 단순하고 안전하다.
	ListView_Rooms->ClearListItems();

	for (const FMasterLobbyRoomInfo& Room : RoomList)
	{
		ULastFPSMasterLobbyRoomEntry* Entry = NewObject<ULastFPSMasterLobbyRoomEntry>(this);
		Entry->RoomInfo = Room;
		ListView_Rooms->AddItem(Entry);
	}
}

void ULastFPSMasterLobbyWidget::OnCreateRoomClicked()
{
	ALastFPSMasterLobbyPC* LobbyPC = GetLobbyPlayerController();
	if (!LobbyPC)
	{
		return;
	}

	ULastFPSMasterLobbyClientSubsystem* MasterLobby =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<ULastFPSMasterLobbyClientSubsystem>() : nullptr;
	if (!MasterLobby)
	{
		UE_LOG(LogMasterLobby, Error, TEXT("방 개설 실패: 마스터 로비 서브시스템을 찾을 수 없다."));
		return;
	}

	// 제목 입력란이 없으면 빈 문자열을 보내고 서버가 기본 제목을 만든다.
	const FString RequestedRoomName = Txt_RoomNameInput ? Txt_RoomNameInput->GetText().ToString() : FString();

	// 등록은 이동 후 리슨 서버에서 Beacon으로 이뤄진다.
	// 로비에서 미리 등록하면 이동하며 접속이 끊길 때 방이 함께 사라진다.
	MasterLobby->BeginHostingRoom(RequestedRoomName, 0);
}

void ULastFPSMasterLobbyWidget::OnRefreshRoomsClicked()
{
	if (ALastFPSMasterLobbyPC* LobbyPC = GetLobbyPlayerController())
	{
		LobbyPC->Server_RequestRoomList();
	}
}
