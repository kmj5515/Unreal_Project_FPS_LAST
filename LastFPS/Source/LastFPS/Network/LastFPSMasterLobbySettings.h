#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LastFPSMasterLobbySettings.generated.h"

/**
 * 마스터 로비 접속과 방 개설에 필요한 클라이언트 측 설정이다.
 *
 * 주소를 코드에 박으면 서버 IP가 바뀔 때마다 클라이언트를 다시 빌드해야 하므로,
 * 설정과 실행 인자만으로 바꿀 수 있게 분리한다. 우선순위는 실행 인자 > 설정값이다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "LastFPS Master Lobby"))
class LASTFPS_API ULastFPSMasterLobbySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * 마스터 로비 서버 주소다. IP 또는 호스트 이름을 넣는다.
	 * 사설 IP를 넣으면 같은 네트워크 안에서만 접속되므로, 외부 배포본은
	 * 공인 IP나 도메인으로 바꾸거나 아래 실행 인자로 덮어써야 한다.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Master Lobby")
	FString ServerAddress = TEXT("172.30.1.15");

	/** 마스터 로비 서버의 게임 포트다. 서버 자동화의 GamePort와 같아야 한다. */
	UPROPERTY(Config, EditAnywhere, Category = "Master Lobby", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 ServerPort = 7777;

	/**
	 * 방 정보를 보고할 Beacon 포트다. 게임 포트와 달라야 하며
	 * 마스터 로비 GameMode의 BeaconPort와 같아야 한다.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Master Lobby", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 BeaconPort = 15000;

	/**
	 * 실행 인자로 주소를 덮어쓸 때 사용할 키다.
	 * 예) LastFPS.exe -MasterServer=203.0.113.10:7777
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Master Lobby")
	FString CommandLineOverrideKey = TEXT("MasterServer");

	/** 방을 개설한 호스트가 리슨 서버로 열 맵이다. 비어 있으면 방 개설이 실패한다. */
	UPROPERTY(Config, EditAnywhere, Category = "Room")
	TSoftObjectPtr<UWorld> HostedRoomMap;

	/** 호스트가 리슨 서버를 열 UDP 포트다. 마스터 로비 GameMode의 HostGamePort와 같아야 한다. */
	UPROPERTY(Config, EditAnywhere, Category = "Room", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 HostGamePort = 7777;

	/** UI가 인원 수를 지정하지 않았을 때 사용할 기본 최대 인원이다. */
	UPROPERTY(Config, EditAnywhere, Category = "Room", meta = (ClampMin = "1"))
	int32 DefaultRoomMaxPlayers = 4;

	/**
	 * 마스터 로비 게임 연결 주소를 만든다. 실행 인자가 있으면 그 값이 설정값을 대신한다.
	 * @param OutUrl   "주소:포트" 형식의 접속 URL
	 * @param OutError 실패 사유. 실패했을 때만 채워진다.
	 */
	bool BuildConnectUrl(FString& OutUrl, FString& OutError) const;

	/**
	 * 방 정보를 보고할 Beacon 연결 주소를 만든다.
	 * 게임 주소와 같은 호스트를 쓰되 포트만 BeaconPort로 바꾼다.
	 */
	bool BuildBeaconUrl(FString& OutUrl, FString& OutError) const;

private:
	/** 실행 인자와 설정값 중 실제로 사용할 주소에서 호스트 부분만 뽑는다. */
	bool ResolveHostAndPort(FString& OutHost, int32& OutPort, FString& OutError) const;
};
