#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "LastFPSMasterLobbyBeaconClient.generated.h"

/**
 * 방을 개설한 호스트가 마스터 로비 서버와 유지하는 경량 연결이다.
 *
 * 호스트는 자기 PC에서 리슨 서버를 열어야 하므로 마스터 로비의 게임 연결을 유지할 수 없다.
 * Beacon은 게임 넷드라이버와 별도의 넷드라이버를 쓰므로, 리슨 서버를 돌리면서도
 * 마스터 로비에 방 정보를 계속 보고할 수 있다.
 *
 * 이 액터는 호스트(클라이언트 역할)와 마스터 서버(서버 역할) 양쪽에 존재한다.
 */
UCLASS(Transient, NotPlaceable)
class LASTFPS_API ALastFPSMasterLobbyBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()

public:
	ALastFPSMasterLobbyBeaconClient(const FObjectInitializer& ObjectInitializer);

	/**
	 * 마스터 로비 서버에 Beacon 연결을 시작한다.
	 * @param MasterAddress "주소:비컨포트" 형식
	 * @return 연결 시도를 시작했으면 true.
	 */
	bool ConnectToMasterLobby(const FString& MasterAddress);

	/** 연결 성공 후 이 호스트의 방 정보를 최초 등록할 때 사용할 값을 미리 넣어 둔다. */
	void SetPendingRoom(const FString& InRoomName, int32 InMaxPlayers);

	/** 하트비트로 보낼 현재 인원을 갱신한다. */
	void UpdateCurrentPlayers(int32 InCurrentPlayers);

	/** 서버 전용. 이 연결로 등록된 방의 접속 주소이며, 등록 전이면 빈 문자열이다. */
	const FString& GetServerSideHostAddress() const { return ServerSideHostAddress; }

	// AOnlineBeaconClient
	virtual void OnConnected() override;
	virtual void OnFailure() override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 호스트 → 마스터. 방을 등록하거나 이미 있으면 갱신한다. */
	UFUNCTION(Server, Reliable)
	void ServerRegisterRoom(const FString& RoomName, int32 MaxPlayers, int32 CurrentPlayers);

	/** 호스트 → 마스터. 방이 살아 있음을 알리고 인원을 갱신한다. */
	UFUNCTION(Server, Reliable)
	void ServerHeartbeat(int32 CurrentPlayers);

private:
	void SendHeartbeat();

	// 마스터 서버에서 이 연결의 원격 주소로 만든 방 식별자다. 서버에서만 채워진다.
	FString ServerSideHostAddress;

	FString PendingRoomName;
	int32 PendingMaxPlayers = 4;
	int32 CachedCurrentPlayers = 1;

	FTimerHandle HeartbeatTimerHandle;
};
