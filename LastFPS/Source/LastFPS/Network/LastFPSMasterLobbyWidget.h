#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSMasterLobbyTypes.h"
#include "LastFPSMasterLobbyWidget.generated.h"

class UButton;
class UEditableTextBox;
class UListView;
class ALastFPSMasterLobbyPC;

/**
 * 마스터 로비 전용 UI 위젯이다.
 * 방 목록 표시와 사용자 입력 전달만 담당하고, 상태 판단은 서버가 한다.
 * 갱신은 Tick 폴링이 아니라 PlayerController의 델리게이트로 받는다.
 */
UCLASS()
class LASTFPS_API ULastFPSMasterLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_CreateRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_RefreshRooms;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UListView> ListView_Rooms;

	// 방 제목 입력란. 없으면 서버가 호스트 이름으로 기본 제목을 만든다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> Txt_RoomNameInput;

private:
	UFUNCTION()
	void OnCreateRoomClicked();

	UFUNCTION()
	void OnRefreshRoomsClicked();

	// 델리게이트로 전달된 최신 목록을 ListView 항목으로 다시 채운다.
	void HandleRoomListUpdated(const TArray<FMasterLobbyRoomInfo>& RoomList);

	ALastFPSMasterLobbyPC* GetLobbyPlayerController() const;

	// 구독 해제를 위해 등록 시점의 컨트롤러와 핸들을 함께 보관한다.
	TWeakObjectPtr<ALastFPSMasterLobbyPC> BoundLobbyPC;
	FDelegateHandle RoomListUpdatedHandle;
};
