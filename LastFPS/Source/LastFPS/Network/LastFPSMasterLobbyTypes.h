#pragma once

#include "CoreMinimal.h"
#include "LastFPSMasterLobbyTypes.generated.h"

// 마스터 로비 서버는 화면 없이 장시간 구동되므로 로그를 LogTemp와 분리해 추적한다.
LASTFPS_API DECLARE_LOG_CATEGORY_EXTERN(LogMasterLobby, Log, All);

/**
 * 마스터 로비 서버가 유지하는 방(리슨 서버 호스트) 정보다.
 * 서버가 소유하는 값만 담고, 클라이언트가 직접 채우지 않는다.
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FMasterLobbyRoomInfo
{
	GENERATED_BODY()

public:
	// 방을 개설한 호스트의 표시 이름
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString HostName;

	// 방 제목
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString RoomName;

	/**
	 * 다른 클라이언트가 `open`에 그대로 넘길 수 있는 "IP:Port" 형식의 접속 주소다.
	 * 서버가 연결 정보에서 직접 만들며, 클라이언트가 보낸 값을 신뢰하지 않는다.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString HostAddress;

	// 현재 접속 인원
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 CurrentPlayers = 1;

	// 최대 수용 인원
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 MaxPlayers = 4;
};
