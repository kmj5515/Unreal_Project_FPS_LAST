#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LastFPSGameInstance.generated.h"

UCLASS(Config=Game)
class LASTFPS_API ULastFPSGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    virtual int32 AddLocalPlayer(ULocalPlayer* NewPlayer, FPlatformUserId UserId) override;
    virtual bool RemoveLocalPlayer(ULocalPlayer* ExistingPlayer) override;

    void SaveSelectedCharacterIndex(const FString& PlayerKey, int32 SelectedIndex);
    bool TryGetSelectedCharacterIndex(const FString& PlayerKey, int32& OutSelectedIndex) const;

    /** 로비 → 매치 ServerTravel. 로딩 화면은 CommonLoadingScreen이 자동 처리. */
    UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
    void RequestTravelToMatch(const FString& MatchMapURL);

    /** 매치 → 로비 ServerTravel. */
    UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
    void RequestTravelToLobby(const FString& LobbyMapURL);

private:
    UPROPERTY()
    TMap<FString, int32> SelectedCharacterIndexByPlayerKey;
};
