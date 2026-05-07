#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LastFPSGameModeBase.generated.h"

class ALastFPSPlayerState;

UENUM(BlueprintType)
enum class ELastFPSTeam : uint8
{
    TeamA = 0,
    TeamB = 1,
    TeamC = 2,
    TeamD = 3,
    None  = 255
};

UCLASS()
class LASTFPS_API ALastFPSGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALastFPSGameModeBase();

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Teams")
    int32 GetTeamPlayerCount(ELastFPSTeam Team) const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Teams")
    bool IsTeamFull(ELastFPSTeam Team) const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Teams")
    int32 GetTotalConnectedPlayers() const;

    // 팀 스폰 포인트 — 에디터에서 팀별 위치를 할당
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Teams")
    TMap<ELastFPSTeam, TSoftObjectPtr<AActor>> TeamSpawnPoints;

    // 캐릭터 선택 인덱스별 Pawn 클래스 목록 (로비/매치 GM에서 동일 순서로 세팅)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Character")
    TArray<TSubclassOf<APawn>> CharacterPawnClasses;

protected:
    // 팀당 최대 인원 (기본 3)
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Teams")
    int32 MaxPlayersPerTeam = 3;

    // 최대 팀 수 (기본 4)
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Teams")
    int32 MaxTeams = 4;

    // 신규 플레이어에게 인원이 가장 적은 팀 배정
    ELastFPSTeam AssignTeam() const;

    // 팀별 현재 인원 집계
    TMap<ELastFPSTeam, TArray<TWeakObjectPtr<AController>>> TeamRoster;
};
