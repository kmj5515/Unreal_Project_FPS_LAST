#include "LastFPSMasterLobbyBeaconHostObject.h"
#include "LastFPSMasterLobbyBeaconClient.h"
#include "LastFPSMasterLobbyGameMode.h"
#include "LastFPSMasterLobbyTypes.h"
#include "Engine/World.h"

ALastFPSMasterLobbyBeaconHostObject::ALastFPSMasterLobbyBeaconHostObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ClientBeaconActorClass = ALastFPSMasterLobbyBeaconClient::StaticClass();
	// BeaconTypeName은 클라이언트와 문자열이 일치해야 연결이 수락된다.
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

void ALastFPSMasterLobbyBeaconHostObject::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	// 호스트가 정상 종료했으면 TTL 만료를 기다리지 않고 즉시 목록에서 뺀다.
	if (const ALastFPSMasterLobbyBeaconClient* LobbyBeacon = Cast<ALastFPSMasterLobbyBeaconClient>(LeavingClientActor))
	{
		UWorld* World = GetWorld();
		if (ALastFPSMasterLobbyGameMode* LobbyGameMode = World ? World->GetAuthGameMode<ALastFPSMasterLobbyGameMode>() : nullptr)
		{
			LobbyGameMode->RemoveRoom(LobbyBeacon->GetServerSideHostAddress());
		}
	}

	Super::NotifyClientDisconnected(LeavingClientActor);
}
