#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LastFPSMasterLobbyTypes.h"
#include "LastFPSMasterLobbyRoomEntry.generated.h"

/**
 * ListView는 UObject 항목만 받으므로 FMasterLobbyRoomInfo를 감싸는 표시용 래퍼다.
 * 서버 권한 상태를 담지 않는 읽기 전용 스냅샷이다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSMasterLobbyRoomEntry : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FMasterLobbyRoomInfo RoomInfo;
};
