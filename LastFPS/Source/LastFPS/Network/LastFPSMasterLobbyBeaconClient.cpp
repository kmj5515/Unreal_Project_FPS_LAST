#include "LastFPSMasterLobbyBeaconClient.h"
#include "LastFPSMasterLobbyGameMode.h"
#include "LastFPSMasterLobbyTypes.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	// 하트비트 주기. GameMode의 RoomTimeoutSeconds보다 충분히 짧아야 한다.
	constexpr float MasterLobbyHeartbeatIntervalSeconds = 10.0f;
}

ALastFPSMasterLobbyBeaconClient::ALastFPSMasterLobbyBeaconClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALastFPSMasterLobbyBeaconClient::SetPendingRoom(const FString& InRoomName, int32 InMaxPlayers)
{
	PendingRoomName = InRoomName;
	PendingMaxPlayers = InMaxPlayers;
}

void ALastFPSMasterLobbyBeaconClient::UpdateCurrentPlayers(int32 InCurrentPlayers)
{
	CachedCurrentPlayers = FMath::Max(0, InCurrentPlayers);
}

bool ALastFPSMasterLobbyBeaconClient::ConnectToMasterLobby(const FString& MasterAddress)
{
	if (MasterAddress.IsEmpty())
	{
		UE_LOG(LogMasterLobby, Error, TEXT("Beacon 연결 실패: 마스터 주소가 비어 있다."));
		return false;
	}

	FURL ConnectUrl(nullptr, *MasterAddress, TRAVEL_Absolute);
	if (!InitClient(ConnectUrl))
	{
		UE_LOG(LogMasterLobby, Error, TEXT("Beacon 연결 시작 실패. Address=%s"), *MasterAddress);
		return false;
	}

	UE_LOG(LogMasterLobby, Log, TEXT("Beacon 연결 시도. Address=%s"), *MasterAddress);
	return true;
}

void ALastFPSMasterLobbyBeaconClient::OnConnected()
{
	Super::OnConnected();

	UE_LOG(LogMasterLobby, Log, TEXT("Beacon 연결 성공. 방을 등록한다. Name=%s"), *PendingRoomName);

	ServerRegisterRoom(PendingRoomName, PendingMaxPlayers, CachedCurrentPlayers);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HeartbeatTimerHandle, this, &ALastFPSMasterLobbyBeaconClient::SendHeartbeat,
			MasterLobbyHeartbeatIntervalSeconds, true);
	}
}

void ALastFPSMasterLobbyBeaconClient::OnFailure()
{
	UE_LOG(LogMasterLobby, Warning, TEXT("Beacon 연결 실패. 마스터 서버가 꺼져 있거나 Beacon 포트가 막혀 있을 수 있다."));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
	}

	Super::OnFailure();
}

void ALastFPSMasterLobbyBeaconClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ALastFPSMasterLobbyBeaconClient::SendHeartbeat()
{
	ServerHeartbeat(CachedCurrentPlayers);
}

void ALastFPSMasterLobbyBeaconClient::ServerRegisterRoom_Implementation(const FString& RoomName, int32 MaxPlayers, int32 CurrentPlayers)
{
	UWorld* World = GetWorld();
	ALastFPSMasterLobbyGameMode* LobbyGameMode = World ? World->GetAuthGameMode<ALastFPSMasterLobbyGameMode>() : nullptr;
	if (!LobbyGameMode)
	{
		UE_LOG(LogMasterLobby, Warning, TEXT("Beacon 방 등록 무시: 마스터 로비 GameMode가 없다."));
		return;
	}

	// 주소는 클라이언트가 보낸 값을 쓰지 않고 서버가 연결 정보에서 직접 만든다.
	// LowLevelGetRemoteAddress가 비const라 연결 포인터도 const로 둘 수 없다.
	UNetConnection* Connection = GetNetConnection();
	if (!Connection)
	{
		UE_LOG(LogMasterLobby, Warning, TEXT("Beacon 방 등록 무시: 연결 정보를 찾을 수 없다."));
		return;
	}

	// Beacon 포트가 아니라 호스트가 실제로 게임을 여는 포트를 붙여야 다른 클라이언트가 접속할 수 있다.
	const FString HostIp = Connection->LowLevelGetRemoteAddress(false);
	if (HostIp.IsEmpty())
	{
		UE_LOG(LogMasterLobby, Warning, TEXT("Beacon 방 등록 무시: 원격 주소가 비어 있다."));
		return;
	}

	ServerSideHostAddress = FString::Printf(TEXT("%s:%d"), *HostIp, LobbyGameMode->GetHostGamePort());
	LobbyGameMode->RegisterOrUpdateRoom(ServerSideHostAddress, RoomName, MaxPlayers, CurrentPlayers);
}

void ALastFPSMasterLobbyBeaconClient::ServerHeartbeat_Implementation(int32 CurrentPlayers)
{
	if (ServerSideHostAddress.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	ALastFPSMasterLobbyGameMode* LobbyGameMode = World ? World->GetAuthGameMode<ALastFPSMasterLobbyGameMode>() : nullptr;
	if (!LobbyGameMode)
	{
		return;
	}

	LobbyGameMode->HeartbeatRoom(ServerSideHostAddress, CurrentPlayers);
}
