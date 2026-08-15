#include "LastFPSMasterLobbyGameMode.h"
#include "LastFPSMasterLobbyBeaconHostObject.h"
#include "LastFPSMasterLobbyPC.h"
#include "Engine/World.h"
#include "OnlineBeaconHost.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogMasterLobby);

ALastFPSMasterLobbyGameMode::ALastFPSMasterLobbyGameMode()
{
	// 로비는 Pawn 없이 UI만 사용하므로 기본 Pawn 스폰을 막는다.
	PlayerControllerClass = ALastFPSMasterLobbyPC::StaticClass();
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;

	// 방 만료 검사는 타이머로 처리한다. 매 프레임 확인할 이유가 없다.
	PrimaryActorTick.bCanEverTick = false;
}

void ALastFPSMasterLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	StartBeaconHost();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SweepTimerHandle, this, &ALastFPSMasterLobbyGameMode::SweepExpiredRooms,
			RoomSweepIntervalSeconds, true);
	}
}

void ALastFPSMasterLobbyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SweepTimerHandle);
	}

	if (BeaconHost)
	{
		BeaconHost->DestroyBeacon();
		BeaconHost = nullptr;
	}
	BeaconHostObject = nullptr;

	Super::EndPlay(EndPlayReason);
}

void ALastFPSMasterLobbyGameMode::StartBeaconHost()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	BeaconHost = World->SpawnActor<AOnlineBeaconHost>(AOnlineBeaconHost::StaticClass());
	if (!BeaconHost)
	{
		UE_LOG(LogMasterLobby, Error, TEXT("Beacon 호스트 액터 생성에 실패했다. 호스트가 방을 등록할 수 없다."));
		return;
	}

	BeaconHost->ListenPort = BeaconPort;
	if (!BeaconHost->InitHost())
	{
		UE_LOG(LogMasterLobby, Error, TEXT("Beacon 호스트 초기화 실패. Port=%d 가 이미 사용 중인지 확인할 것."), BeaconPort);
		BeaconHost->DestroyBeacon();
		BeaconHost = nullptr;
		return;
	}

	BeaconHostObject = World->SpawnActor<ALastFPSMasterLobbyBeaconHostObject>(ALastFPSMasterLobbyBeaconHostObject::StaticClass());
	if (!BeaconHostObject)
	{
		UE_LOG(LogMasterLobby, Error, TEXT("Beacon 호스트 오브젝트 생성에 실패했다."));
		BeaconHost->DestroyBeacon();
		BeaconHost = nullptr;
		return;
	}

	BeaconHost->RegisterHost(BeaconHostObject);
	BeaconHost->PauseBeaconRequests(false);

	UE_LOG(LogMasterLobby, Log, TEXT("Beacon 호스트 기동. Port=%d, RoomTimeout=%.0fs"), BeaconPort, RoomTimeoutSeconds);
}

ALastFPSMasterLobbyGameMode::FRoomRecord* ALastFPSMasterLobbyGameMode::FindRoom(const FString& HostAddress)
{
	return Rooms.FindByPredicate([&HostAddress](const FRoomRecord& Record)
	{
		return Record.Info.HostAddress == HostAddress;
	});
}

bool ALastFPSMasterLobbyGameMode::RegisterOrUpdateRoom(const FString& HostAddress, const FString& RoomName, int32 MaxPlayers, int32 CurrentPlayers)
{
	if (HostAddress.IsEmpty())
	{
		UE_LOG(LogMasterLobby, Warning, TEXT("방 등록 거부: 호스트 주소를 확인할 수 없다."));
		return false;
	}

	const double Now = FPlatformTime::Seconds();
	const int32 ClampedMax = FMath::Clamp(MaxPlayers, MinPlayersPerRoom, MaxPlayersPerRoom);

	if (FRoomRecord* Existing = FindRoom(HostAddress))
	{
		Existing->Info.RoomName = SanitizeRoomName(RoomName);
		Existing->Info.MaxPlayers = ClampedMax;
		Existing->Info.CurrentPlayers = FMath::Clamp(CurrentPlayers, 0, ClampedMax);
		Existing->LastHeartbeatSeconds = Now;
		UE_LOG(LogMasterLobby, Log, TEXT("방 갱신: Name=%s, Address=%s"), *Existing->Info.RoomName, *HostAddress);
		PublishRoomList();
		return true;
	}

	if (Rooms.Num() >= MaxActiveRooms)
	{
		UE_LOG(LogMasterLobby, Warning, TEXT("방 등록 거부: 활성 방 수가 상한(%d)에 도달했다. Address=%s"), MaxActiveRooms, *HostAddress);
		return false;
	}

	FRoomRecord Record;
	Record.Info.HostAddress = HostAddress;
	Record.Info.RoomName = SanitizeRoomName(RoomName);
	Record.Info.MaxPlayers = ClampedMax;
	Record.Info.CurrentPlayers = FMath::Clamp(CurrentPlayers, 0, ClampedMax);
	Record.LastHeartbeatSeconds = Now;
	Rooms.Add(MoveTemp(Record));

	UE_LOG(LogMasterLobby, Log, TEXT("방 등록: Name=%s, Address=%s, Max=%d"), *Rooms.Last().Info.RoomName, *HostAddress, ClampedMax);
	PublishRoomList();
	return true;
}

bool ALastFPSMasterLobbyGameMode::HeartbeatRoom(const FString& HostAddress, int32 CurrentPlayers)
{
	FRoomRecord* Record = FindRoom(HostAddress);
	if (!Record)
	{
		return false;
	}

	Record->LastHeartbeatSeconds = FPlatformTime::Seconds();

	const int32 NewCount = FMath::Clamp(CurrentPlayers, 0, Record->Info.MaxPlayers);
	if (Record->Info.CurrentPlayers == NewCount)
	{
		// 인원 변화가 없으면 목록을 다시 보내지 않는다. 하트비트마다 전체 브로드캐스트할 이유가 없다.
		return true;
	}

	Record->Info.CurrentPlayers = NewCount;
	PublishRoomList();
	return true;
}

bool ALastFPSMasterLobbyGameMode::RemoveRoom(const FString& HostAddress)
{
	if (HostAddress.IsEmpty())
	{
		return false;
	}

	const int32 RemovedCount = Rooms.RemoveAll([&HostAddress](const FRoomRecord& Record)
	{
		return Record.Info.HostAddress == HostAddress;
	});

	if (RemovedCount <= 0)
	{
		return false;
	}

	UE_LOG(LogMasterLobby, Log, TEXT("방 제거: Address=%s"), *HostAddress);
	PublishRoomList();
	return true;
}

void ALastFPSMasterLobbyGameMode::SweepExpiredRooms()
{
	if (Rooms.Num() == 0)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	const double Timeout = static_cast<double>(RoomTimeoutSeconds);

	const int32 RemovedCount = Rooms.RemoveAll([Now, Timeout](const FRoomRecord& Record)
	{
		return (Now - Record.LastHeartbeatSeconds) > Timeout;
	});

	if (RemovedCount > 0)
	{
		// 호스트가 강제 종료되면 Beacon 해제 통지가 오지 않으므로 이 경로로만 정리된다.
		UE_LOG(LogMasterLobby, Log, TEXT("하트비트 만료로 방 %d개를 제거했다. Timeout=%.0fs"), RemovedCount, RoomTimeoutSeconds);
		PublishRoomList();
	}
}

void ALastFPSMasterLobbyGameMode::PublishRoomList()
{
	PublishedRooms.Reset(Rooms.Num());
	for (const FRoomRecord& Record : Rooms)
	{
		PublishedRooms.Add(Record.Info);
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 리슨 서버의 로컬 PlayerController도 포함한다. Client RPC는 로컬에서 직접 실행된다.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALastFPSMasterLobbyPC* LobbyPC = Cast<ALastFPSMasterLobbyPC>(It->Get()))
		{
			LobbyPC->Client_ReceiveRoomList(PublishedRooms);
		}
	}
}

FString ALastFPSMasterLobbyGameMode::SanitizeRoomName(const FString& RawRoomName) const
{
	FString Trimmed = RawRoomName;
	Trimmed.TrimStartAndEndInline();

	if (Trimmed.IsEmpty())
	{
		Trimmed = FString::Format(*FallbackRoomNameFormat, { TEXT("Host") });
	}

	if (Trimmed.Len() > MaxRoomNameLength)
	{
		Trimmed.LeftInline(MaxRoomNameLength, EAllowShrinking::No);
	}

	return Trimmed;
}
