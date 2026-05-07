#pragma once

#include "CoreMinimal.h"
#include "Game/LastFPSGameModeBase.h"
#include "TimerManager.h"
#include "LastFPSLobbyGameMode.generated.h"

UCLASS()
class LASTFPS_API ALastFPSLobbyGameMode : public ALastFPSGameModeBase
{
    GENERATED_BODY()

public:
    ALastFPSLobbyGameMode();
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    void SetPlayerReady(AController* PlayerController, bool bReady);

    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    int32 GetRemainingCharacterSelectSeconds() const { return RemainingCharacterSelectSeconds; }

    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    bool IsCharacterSelectInProgress() const { return bCharacterSelectInProgress; }

    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    bool IsTeamIntroInProgress() const { return bTeamIntroInProgress; }

    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    bool IsTravelTriggered() const { return bLobbyMatchStartTriggered; }

    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    int32 GetLobbyStartPlayerCount() const { return LobbyStartPlayerCount; }

protected:
    void TryStartMatchFromLobby();
    void SyncLobbyStateToGameState();
    void StartCharacterSelectPhase();
    void TickCharacterSelectPhase();
    bool AreAllPlayersReady() const;
    int32 GetValidLobbyPlayerCount() const;
    void StartTeamIntroPhase();
    void FinishTeamIntroPhase();
    void ExecuteMatchTravel();
    void DebugLobbyFlow(const FString& Message, FColor Color = FColor::Green) const;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Lobby")
    int32 LobbyStartPlayerCount = 3;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Lobby")
    FString MatchMapURL;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Lobby")
    int32 CharacterSelectSeconds = 30;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Lobby")
    float TeamIntroSeconds = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Lobby")
    TSubclassOf<APawn> LobbyPawnClass;

    bool bCharacterSelectInProgress = false;
    bool bTeamIntroInProgress = false;
    bool bLobbyMatchStartTriggered = false;

    int32 RemainingCharacterSelectSeconds = 0;
    FTimerHandle CharacterSelectTimerHandle;
    FTimerHandle TeamIntroTimerHandle;
    TMap<TWeakObjectPtr<AController>, bool> PlayerReadyMap;
};
