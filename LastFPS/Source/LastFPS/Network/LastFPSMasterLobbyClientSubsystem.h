#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LastFPSMasterLobbyClientSubsystem.generated.h"

class ALastFPSMasterLobbyBeaconClient;

/**
 * 클라이언트에서 마스터 로비와 관련된 모든 접속 흐름을 담당한다.
 *
 * - 마스터 로비 서버로 접속
 * - 방 개설: 자기 PC에서 리슨 서버를 열고, Beacon으로 마스터 로비에 방을 보고
 *
 * 호스트는 방을 열기 위해 마스터 로비의 게임 연결을 끊어야 하므로,
 * 맵 이동을 넘어 살아남는 GameInstance 수명에서 이 흐름을 관리해야 한다.
 */
UCLASS()
class LASTFPS_API ULastFPSMasterLobbyClientSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 설정된 마스터 로비 서버로 접속을 시작한다.
	 * @return 접속 요청을 보냈으면 true. 주소가 잘못됐거나 컨트롤러가 없으면 false.
	 */
	UFUNCTION(BlueprintCallable, Category = "LastFPS|MasterLobby")
	bool ConnectToMasterLobby();

	/**
	 * 방을 개설한다. 호스트 맵을 리슨 서버로 연 뒤 Beacon으로 마스터 로비에 등록한다.
	 * @return 이동을 시작했으면 true. 호스트 맵이 설정되지 않았으면 false.
	 */
	UFUNCTION(BlueprintCallable, Category = "LastFPS|MasterLobby")
	bool BeginHostingRoom(const FString& RoomName, int32 MaxPlayers);

	/** 방 개설을 중단하고 Beacon 연결을 끊는다. 마스터 로비의 방도 즉시 제거된다. */
	UFUNCTION(BlueprintCallable, Category = "LastFPS|MasterLobby")
	void StopHostingRoom();

	/** 현재 이 클라이언트가 방을 호스팅 중인지 여부다. */
	UFUNCTION(BlueprintPure, Category = "LastFPS|MasterLobby")
	bool IsHostingRoom() const;

	/** 현재 적용될 마스터 로비 접속 주소다. UI 표시나 로그 확인용이다. */
	UFUNCTION(BlueprintPure, Category = "LastFPS|MasterLobby")
	FString GetEffectiveServerAddress() const;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 데디케이티드 서버 프로세스에는 접속할 대상이 없으므로 생성하지 않는다.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

private:
	/** 호스트 맵 로드가 끝난 시점에 Beacon을 붙인다. 이동 전에는 리슨 서버가 아직 없다. */
	void HandlePostLoadMap(UWorld* LoadedWorld);

	void StartRoomBeacon(UWorld* HostWorld);

	/** 리슨 서버의 현재 접속 인원을 Beacon에 반영한다. */
	void RefreshHostedPlayerCount();

	UPROPERTY(Transient)
	TObjectPtr<ALastFPSMasterLobbyBeaconClient> RoomBeacon;

	// 이동 후 등록할 방 정보다. 이동 전에 채워 두고 맵 로드 완료 시 사용한다.
	FString PendingRoomName;
	int32 PendingMaxPlayers = 4;
	bool bHostingRequested = false;

	FDelegateHandle PostLoadMapHandle;
	FTimerHandle PlayerCountTimerHandle;
};
