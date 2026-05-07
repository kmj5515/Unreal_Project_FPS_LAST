#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LastFPSPlayerController.generated.h"

class APawn;

UCLASS()
class LASTFPS_API ALastFPSPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    void SetLobbyReady(bool bReady);

    UFUNCTION(BlueprintPure, Category="LastFPS|Lobby")
    bool IsLobbyReady() const { return bLobbyReady; }

    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    void SetSelectedCharacterIndex(int32 NewIndex);

    UFUNCTION(BlueprintPure, Category="LastFPS|Lobby")
    int32 GetSelectedCharacterIndex() const { return SelectedCharacterIndex; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Lobby")
    TSubclassOf<APawn> GetSelectedCharacterClass() const;

    UFUNCTION(BlueprintPure, Category="LastFPS|Lobby")
    const TArray<TSubclassOf<APawn>>& GetSelectableCharacterClasses() const { return SelectableCharacterClasses; }

    UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Lobby")
    void OnSelectedCharacterIndexChanged(int32 NewSelectedCharacterIndex);

protected:
    int32 ClampSelectedCharacterIndex(int32 NewIndex) const;
    void SyncSelectedCharacterState(int32 CharacterIndex);

    UFUNCTION(Server, Reliable)
    void ServerSetLobbyReady(bool bReady);

    UFUNCTION(Server, Reliable)
    void ServerSetSelectedCharacterIndex(int32 NewIndex);

    UFUNCTION()
    void OnRep_SelectedCharacterIndex();

    UPROPERTY(BlueprintReadOnly, Category="LastFPS|Lobby")
    bool bLobbyReady = false;

    // 캐릭터 선택 UI에서 사용할 후보 Pawn 클래스 목록(BP에서 세팅)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Lobby")
    TArray<TSubclassOf<APawn>> SelectableCharacterClasses;

    UPROPERTY(ReplicatedUsing=OnRep_SelectedCharacterIndex, BlueprintReadOnly, Category="LastFPS|Lobby")
    int32 SelectedCharacterIndex = 0;
};
