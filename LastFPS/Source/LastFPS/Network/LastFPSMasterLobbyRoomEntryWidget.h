#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "LastFPSMasterLobbyRoomEntryWidget.generated.h"

class UButton;
class UTextBlock;
class ULastFPSMasterLobbyRoomEntry;

/**
 * 마스터 로비 방 목록의 한 줄을 담당하는 항목 위젯이다.
 * 표시와 접속 요청만 하며, 방 정보를 직접 수정하지 않는다.
 */
UCLASS()
class LASTFPS_API ULastFPSMasterLobbyRoomEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_RoomName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_HostName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_PlayerCount;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_JoinRoom;

	// Blueprint에서 추가 표현이 필요할 때만 사용한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void BP_OnRoomEntryUpdated(ULastFPSMasterLobbyRoomEntry* Entry);

private:
	UFUNCTION()
	void OnJoinRoomClicked();

	void RefreshDisplay();

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSMasterLobbyRoomEntry> BoundEntry;
};
