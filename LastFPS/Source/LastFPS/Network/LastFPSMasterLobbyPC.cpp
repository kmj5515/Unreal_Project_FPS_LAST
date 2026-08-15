#include "LastFPSMasterLobbyPC.h"
#include "LastFPSMasterLobbyGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Misc/App.h"

void ALastFPSMasterLobbyPC::BeginPlay()
{
	Super::BeginPlay();

	CreateLobbyWidgetForLocalPlayer();

	// 로컬 클라이언트는 진입 즉시 현재 목록을 받아야 한다.
	if (IsLocalPlayerController())
	{
		Server_RequestRoomList();
	}
}

void ALastFPSMasterLobbyPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 위젯 수명은 이 컨트롤러가 소유한다. 맵 이동 시 뷰포트에 남지 않도록 정리한다.
	if (LobbyWidget)
	{
		LobbyWidget->RemoveFromParent();
		LobbyWidget = nullptr;
	}

	OnRoomListUpdatedNative.Clear();

	Super::EndPlay(EndPlayReason);
}

void ALastFPSMasterLobbyPC::CreateLobbyWidgetForLocalPlayer()
{
	// 로컬 플레이어가 아니면(서버가 들고 있는 원격 클라이언트 프록시) UI 대상이 아니다.
	if (!IsLocalPlayerController())
	{
		return;
	}

	// -NullRHI 헤드리스 서버 프로세스에서는 UMG를 생성하지 않는다.
	if (!FApp::CanEverRender())
	{
		return;
	}

	if (LobbyWidgetClass.IsNull())
	{
		UE_LOG(LogMasterLobby, Error,
			TEXT("로비 위젯 클래스가 설정되지 않았다. DefaultGame.ini의 [/Script/LastFPS.LastFPSMasterLobbyPC] LobbyWidgetClass를 지정해야 한다."));
		return;
	}

	// 로비 진입 시 한 번만 발생하는 로드이므로 동기 로드가 허용된다.
	UClass* ResolvedWidgetClass = LobbyWidgetClass.LoadSynchronous();
	if (!ResolvedWidgetClass)
	{
		UE_LOG(LogMasterLobby, Error, TEXT("로비 위젯 클래스를 로드하지 못했다. Path=%s"), *LobbyWidgetClass.ToString());
		return;
	}

	LobbyWidget = CreateWidget<UUserWidget>(this, ResolvedWidgetClass);
	if (!LobbyWidget)
	{
		UE_LOG(LogMasterLobby, Error, TEXT("로비 위젯 생성에 실패했다. Class=%s"), *ResolvedWidgetClass->GetName());
		return;
	}

	LobbyWidget->AddToViewport();

	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
}

void ALastFPSMasterLobbyPC::Server_RequestRoomList_Implementation()
{
	UWorld* World = GetWorld();
	const ALastFPSMasterLobbyGameMode* LobbyGameMode = World ? World->GetAuthGameMode<ALastFPSMasterLobbyGameMode>() : nullptr;
	if (!LobbyGameMode)
	{
		return;
	}

	Client_ReceiveRoomList(LobbyGameMode->GetActiveRooms());
}

void ALastFPSMasterLobbyPC::Client_ReceiveRoomList_Implementation(const TArray<FMasterLobbyRoomInfo>& RoomList)
{
	CachedRoomList = RoomList;

	OnRoomListUpdatedNative.Broadcast(CachedRoomList);
	OnRoomListUpdated(CachedRoomList);
}

bool ALastFPSMasterLobbyPC::TravelToRoom(const FMasterLobbyRoomInfo& Room)
{
	if (Room.HostAddress.IsEmpty())
	{
		UE_LOG(LogMasterLobby, Warning, TEXT("접속할 방의 주소가 비어 있다. RoomName=%s"), *Room.RoomName);
		return false;
	}

	UE_LOG(LogMasterLobby, Log, TEXT("방 접속: %s (%s)"), *Room.RoomName, *Room.HostAddress);
	ClientTravel(Room.HostAddress, TRAVEL_Absolute);
	return true;
}
