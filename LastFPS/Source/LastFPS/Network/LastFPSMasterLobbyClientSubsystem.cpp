#include "LastFPSMasterLobbyClientSubsystem.h"
#include "LastFPSMasterLobbyBeaconClient.h"
#include "LastFPSMasterLobbySettings.h"
#include "LastFPSMasterLobbyTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	// 리슨 서버 인원 확인 주기. Beacon 하트비트 주기보다 촘촘할 필요는 없다.
	constexpr float HostedPlayerCountPollSeconds = 5.0f;
}

bool ULastFPSMasterLobbyClientSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	return !IsRunningDedicatedServer();
}

void ULastFPSMasterLobbyClientSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &ULastFPSMasterLobbyClientSubsystem::HandlePostLoadMap);
}

void ULastFPSMasterLobbyClientSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	StopHostingRoom();

	Super::Deinitialize();
}

FString ULastFPSMasterLobbyClientSubsystem::GetEffectiveServerAddress() const
{
	const ULastFPSMasterLobbySettings* Settings = GetDefault<ULastFPSMasterLobbySettings>();
	FString Url;
	FString Error;
	return Settings->BuildConnectUrl(Url, Error) ? Url : FString();
}

bool ULastFPSMasterLobbyClientSubsystem::IsHostingRoom() const
{
	return RoomBeacon != nullptr;
}

bool ULastFPSMasterLobbyClientSubsystem::ConnectToMasterLobby()
{
	const ULastFPSMasterLobbySettings* Settings = GetDefault<ULastFPSMasterLobbySettings>();

	FString Url;
	FString Error;
	if (!Settings->BuildConnectUrl(Url, Error))
	{
		UE_LOG(LogMasterLobby, Error, TEXT("마스터 로비 접속 실패: %s"), *Error);
		return false;
	}

	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogMasterLobby, Error, TEXT("마스터 로비 접속 실패: 로컬 PlayerController가 없다. Url=%s"), *Url);
		return false;
	}

	// 로비로 돌아갈 때 이전 방 호스팅이 남아 있으면 유령 방이 된다.
	StopHostingRoom();

	UE_LOG(LogMasterLobby, Log, TEXT("마스터 로비 접속 시작: %s"), *Url);
	PC->ClientTravel(Url, TRAVEL_Absolute);
	return true;
}

void ULastFPSMasterLobbyClientSubsystem::SetPendingBattleDefinition(FPrimaryAssetId BattleDefinitionId)
{
	PendingBattleDefinition = BattleDefinitionId;
}

bool ULastFPSMasterLobbyClientSubsystem::BeginHostingRoom(const FString& RoomName, int32 MaxPlayers)
{
	const ULastFPSMasterLobbySettings* Settings = GetDefault<ULastFPSMasterLobbySettings>();

	if (Settings->HostedRoomMap.IsNull())
	{
		UE_LOG(LogMasterLobby, Error,
			TEXT("방 개설 실패: 호스트 맵이 설정되지 않았다. DefaultGame.ini의 [/Script/LastFPS.LastFPSMasterLobbySettings] HostedRoomMap을 지정할 것."));
		return false;
	}

	const FString RoomMapPath = Settings->HostedRoomMap.GetLongPackageName();
	if (RoomMapPath.IsEmpty())
	{
		UE_LOG(LogMasterLobby, Error, TEXT("방 개설 실패: HostedRoomMap 경로가 올바르지 않다. Value=%s"), *Settings->HostedRoomMap.ToString());
		return false;
	}

	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogMasterLobby, Error, TEXT("방 개설 실패: 로컬 PlayerController가 없다."));
		return false;
	}

	// 이동 후 맵 로드가 끝나야 리슨 서버가 존재하므로, 등록 정보만 남기고 이동한다.
	PendingRoomName = RoomName;
	PendingMaxPlayers = MaxPlayers > 0 ? MaxPlayers : Settings->DefaultRoomMaxPlayers;
	bHostingRequested = true;

	FString TravelUrl = FString::Printf(TEXT("%s?listen"), *RoomMapPath);
	if (PendingBattleDefinition.IsValid())
	{
		// 대기실 GameMode가 이 옵션으로 전투 맵을 정한다. 없으면 DefaultBattleMap으로 폴백한다.
		TravelUrl += FString::Printf(TEXT("?BattleDef=%s"), *PendingBattleDefinition.ToString());

		// 다음 방 개설이 지난 선택을 물려받지 않도록 실어 보낸 즉시 비운다.
		PendingBattleDefinition = FPrimaryAssetId();
	}

	UE_LOG(LogMasterLobby, Log, TEXT("방 개설: 리슨 서버로 이동한다. Url=%s, Name=%s"), *TravelUrl, *PendingRoomName);

	PC->ClientTravel(TravelUrl, TRAVEL_Absolute);
	return true;
}

void ULastFPSMasterLobbyClientSubsystem::StopHostingRoom()
{
	bHostingRequested = false;

	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(PlayerCountTimerHandle);
	}

	if (RoomBeacon)
	{
		// DestroyBeacon이 정상 종료를 알리므로 마스터 로비가 TTL을 기다리지 않고 방을 지운다.
		RoomBeacon->DestroyBeacon();
		RoomBeacon = nullptr;
		UE_LOG(LogMasterLobby, Log, TEXT("방 호스팅을 종료했다."));
	}
}

void ULastFPSMasterLobbyClientSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!bHostingRequested || !LoadedWorld)
	{
		return;
	}

	// ?listen이 적용돼 리슨 서버가 됐을 때만 방을 등록한다.
	if (LoadedWorld->GetNetMode() != NM_ListenServer)
	{
		UE_LOG(LogMasterLobby, Warning,
			TEXT("방 개설 취소: 이동한 월드가 리슨 서버가 아니다. NetMode=%d"), static_cast<int32>(LoadedWorld->GetNetMode()));
		bHostingRequested = false;
		return;
	}

	StartRoomBeacon(LoadedWorld);
}

void ULastFPSMasterLobbyClientSubsystem::StartRoomBeacon(UWorld* HostWorld)
{
	const ULastFPSMasterLobbySettings* Settings = GetDefault<ULastFPSMasterLobbySettings>();

	FString BeaconUrl;
	FString Error;
	if (!Settings->BuildBeaconUrl(BeaconUrl, Error))
	{
		UE_LOG(LogMasterLobby, Error, TEXT("방 등록 실패: %s"), *Error);
		bHostingRequested = false;
		return;
	}

	RoomBeacon = HostWorld->SpawnActor<ALastFPSMasterLobbyBeaconClient>(ALastFPSMasterLobbyBeaconClient::StaticClass());
	if (!RoomBeacon)
	{
		UE_LOG(LogMasterLobby, Error, TEXT("방 등록 실패: Beacon 액터를 생성하지 못했다."));
		bHostingRequested = false;
		return;
	}

	RoomBeacon->SetPendingRoom(PendingRoomName, PendingMaxPlayers);
	RefreshHostedPlayerCount();

	if (!RoomBeacon->ConnectToMasterLobby(BeaconUrl))
	{
		RoomBeacon->DestroyBeacon();
		RoomBeacon = nullptr;
		bHostingRequested = false;
		return;
	}

	// 인원 변화는 Beacon 하트비트로 전달된다. 여기서는 값만 최신으로 유지한다.
	HostWorld->GetTimerManager().SetTimer(
		PlayerCountTimerHandle, this, &ULastFPSMasterLobbyClientSubsystem::RefreshHostedPlayerCount,
		HostedPlayerCountPollSeconds, true);
}

void ULastFPSMasterLobbyClientSubsystem::RefreshHostedPlayerCount()
{
	if (!RoomBeacon)
	{
		return;
	}

	const UWorld* World = RoomBeacon->GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	RoomBeacon->UpdateCurrentPlayers(GameState->PlayerArray.Num());
}
