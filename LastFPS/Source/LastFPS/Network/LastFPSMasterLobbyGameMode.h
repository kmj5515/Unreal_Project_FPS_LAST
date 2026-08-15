#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LastFPSMasterLobbyTypes.h"
#include "LastFPSMasterLobbyGameMode.generated.h"

class AOnlineBeaconHost;
class ALastFPSMasterLobbyBeaconHostObject;

/**
 * 마스터 로비 서버(서버 PC에서 상시 구동)의 게임 모드다.
 * 개설된 방 목록만 메모리에 유지하며, 실제 플레이는 각 호스트의 리슨 서버에서 진행된다.
 *
 * 방은 로비 접속이 아니라 호스트의 Beacon 하트비트로 살아 있다고 판단한다.
 * 호스트는 방을 열기 위해 로비에서 나가야 하므로, 로비 접속 종료를 방 삭제 근거로 쓸 수 없다.
 *
 * 상태 변경 권한은 서버에만 있다. 제한값은 DefaultGame.ini에서 조정한다.
 */
UCLASS(Config = Game)
class LASTFPS_API ALastFPSMasterLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALastFPSMasterLobbyGameMode();

	/**
	 * 호스트의 방을 등록하거나 이미 있으면 갱신한다. 하트비트 시각도 함께 갱신된다.
	 * HostAddress는 서버가 Beacon 연결 정보에서 만든 값이어야 한다.
	 * @return 검증을 통과해 목록에 반영됐으면 true.
	 */
	bool RegisterOrUpdateRoom(const FString& HostAddress, const FString& RoomName, int32 MaxPlayers, int32 CurrentPlayers);

	/**
	 * 방이 살아 있음을 갱신하고 현재 인원을 반영한다.
	 * @return 해당 주소의 방을 찾았으면 true.
	 */
	bool HeartbeatRoom(const FString& HostAddress, int32 CurrentPlayers);

	/** 호스트가 정상 종료했을 때 즉시 제거한다. TTL 만료를 기다리지 않는다. */
	bool RemoveRoom(const FString& HostAddress);

	/** 클라이언트에게 내려줄 현재 방 목록이다. */
	const TArray<FMasterLobbyRoomInfo>& GetActiveRooms() const { return PublishedRooms; }

	int32 GetBeaconPort() const { return BeaconPort; }

	/**
	 * 호스트가 리슨 서버를 여는 게임 포트다.
	 * Beacon 연결 포트가 아니라 이 포트를 붙여야 다른 클라이언트가 접속할 수 있다.
	 */
	int32 GetHostGamePort() const { return HostGamePort; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 방 하나가 가질 수 있는 최소 인원
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Limits", meta = (ClampMin = "1"))
	int32 MinPlayersPerRoom = 2;

	// 방 하나가 가질 수 있는 최대 인원
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Limits", meta = (ClampMin = "1"))
	int32 MaxPlayersPerRoom = 4;

	// 방 제목 최대 길이. 초과 시 잘라낸다.
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Limits", meta = (ClampMin = "1"))
	int32 MaxRoomNameLength = 24;

	// 서버가 동시에 유지할 수 있는 방 개수
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Limits", meta = (ClampMin = "1"))
	int32 MaxActiveRooms = 64;

	// 제목이 비어 있을 때 사용할 대체 표기
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Limits")
	FString FallbackRoomNameFormat = TEXT("{0}'s Room");

	/**
	 * 마지막 하트비트 이후 이 시간이 지나면 방을 목록에서 지운다.
	 * 호스트 하트비트 주기보다 넉넉히 커야 일시적인 패킷 손실로 방이 사라지지 않는다.
	 */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Heartbeat", meta = (ClampMin = "5.0"))
	float RoomTimeoutSeconds = 45.0f;

	// 만료된 방을 검사하는 주기다.
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Heartbeat", meta = (ClampMin = "1.0"))
	float RoomSweepIntervalSeconds = 10.0f;

	/**
	 * 호스트 Beacon 연결을 받는 UDP 포트다. 게임 포트와 달라야 하며,
	 * 서버 PC 방화벽과 공유기 포트포워딩에도 이 포트를 열어야 한다.
	 */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Heartbeat", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 BeaconPort = 15000;

	// 호스트가 리슨 서버를 여는 게임 포트다. 클라이언트 설정의 HostGamePort와 같아야 한다.
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Heartbeat", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 HostGamePort = 7777;

private:
	// 서버가 유지하는 방 기록이다. 하트비트 시각은 클라이언트에게 보내지 않는다.
	struct FRoomRecord
	{
		FMasterLobbyRoomInfo Info;
		double LastHeartbeatSeconds = 0.0;
	};

	void StartBeaconHost();
	void SweepExpiredRooms();

	/** 방 목록이 바뀐 뒤 공개 목록을 다시 만들고 접속자 전원에게 전달한다. */
	void PublishRoomList();

	FRoomRecord* FindRoom(const FString& HostAddress);

	// 클라이언트가 보낸 방 제목을 길이 제한과 공백 규칙에 맞게 정리한다.
	FString SanitizeRoomName(const FString& RawRoomName) const;

	TArray<FRoomRecord> Rooms;

	// GetActiveRooms가 매번 배열을 만들지 않도록 갱신 시점에만 다시 만든다.
	TArray<FMasterLobbyRoomInfo> PublishedRooms;

	UPROPERTY(Transient)
	TObjectPtr<AOnlineBeaconHost> BeaconHost;

	UPROPERTY(Transient)
	TObjectPtr<ALastFPSMasterLobbyBeaconHostObject> BeaconHostObject;

	FTimerHandle SweepTimerHandle;
};
