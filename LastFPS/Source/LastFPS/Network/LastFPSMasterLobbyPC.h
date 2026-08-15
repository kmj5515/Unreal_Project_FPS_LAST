#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LastFPSMasterLobbyTypes.h"
#include "LastFPSMasterLobbyPC.generated.h"

class UUserWidget;

/**
 * 마스터 로비에 접속한 클라이언트의 PlayerController다.
 * 로비 UI 표시와 방 목록 수신만 담당한다.
 *
 * 방 등록은 이 컨트롤러가 하지 않는다. 호스트는 방을 열기 위해 로비에서 나가야 하므로,
 * 등록과 하트비트는 맵 이동을 넘어 살아남는 Beacon 연결이 담당한다.
 * (ULastFPSMasterLobbyClientSubsystem / ALastFPSMasterLobbyBeaconClient)
 */
UCLASS(Config = Game)
class LASTFPS_API ALastFPSMasterLobbyPC : public APlayerController
{
	GENERATED_BODY()

public:
	// 방 목록이 갱신될 때 UI가 구독하는 네이티브 델리게이트다. Tick 폴링을 대신한다.
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMasterLobbyRoomListUpdated, const TArray<FMasterLobbyRoomInfo>& /*RoomList*/);
	FOnMasterLobbyRoomListUpdated OnRoomListUpdatedNative;

	// 최신 방 목록을 요청한다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_RequestRoomList();

	// 서버가 소유 클라이언트에게 최신 방 목록을 내려준다.
	UFUNCTION(Client, Reliable, Category = "Lobby")
	void Client_ReceiveRoomList(const TArray<FMasterLobbyRoomInfo>& RoomList);

	// Blueprint UI가 갱신 시점을 잡을 수 있도록 제공한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnRoomListUpdated(const TArray<FMasterLobbyRoomInfo>& UpdatedRoomList);

	// UI가 늦게 생성돼도 마지막 목록을 즉시 그릴 수 있게 캐시를 노출한다.
	const TArray<FMasterLobbyRoomInfo>& GetCachedRoomList() const { return CachedRoomList; }

	// 다른 플레이어가 개설한 방으로 접속한다.
	bool TravelToRoom(const FMasterLobbyRoomInfo& Room);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 로비 UI 위젯 클래스. 항상 메모리에 둘 필요가 없어 소프트 참조로 지정한다.
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|UI")
	TSoftClassPtr<UUserWidget> LobbyWidgetClass;

private:
	// 헤드리스 서버 프로세스에서는 UMG와 입력 모드를 건드리지 않는다.
	void CreateLobbyWidgetForLocalPlayer();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LobbyWidget;

	// 클라이언트가 마지막으로 수신한 방 목록
	TArray<FMasterLobbyRoomInfo> CachedRoomList;
};
