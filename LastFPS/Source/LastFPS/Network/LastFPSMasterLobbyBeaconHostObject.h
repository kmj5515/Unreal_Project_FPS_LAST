#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "LastFPSMasterLobbyBeaconHostObject.generated.h"

/**
 * 마스터 로비 서버에서 호스트들의 Beacon 연결을 받아 주는 객체다.
 * 연결 수립과 해제만 관리하고, 방 목록 자체는 GameMode가 소유한다.
 */
UCLASS(Transient, NotPlaceable)
class LASTFPS_API ALastFPSMasterLobbyBeaconHostObject : public AOnlineBeaconHostObject
{
	GENERATED_BODY()

public:
	ALastFPSMasterLobbyBeaconHostObject(const FObjectInitializer& ObjectInitializer);

	// 연결이 끊긴 호스트의 방을 목록에서 즉시 제거한다. TTL 만료를 기다리지 않는다.
	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
};
