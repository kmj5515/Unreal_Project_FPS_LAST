#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LastFPSPartyGameMode.generated.h"

/**
 * 
 */
UCLASS(Config = Game)
class LASTFPS_API ALastFPSPartyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALastFPSPartyGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	void RequestStartGame(APlayerController* RequestingPlayer);

protected:
	void AssignNewHost();

	bool AreAllPlayersReady() const;

	/**
	 * BattleDef 옵션이 없거나 해석에 실패했을 때 이동할 맵이다.
	 * 대기실에서 시작 버튼을 눌렀을 때의 기본 목적지이므로 코드에 박지 않고 설정에서 받는다.
	 */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Party")
	TSoftObjectPtr<UWorld> DefaultBattleMap;

	/**
	 * 게임을 시작하기 위해 필요한 최소 인원이다. 기본값 1은 방장 혼자서도 시작할 수 있다는 뜻이다.
	 * 파티 강제 인원이 필요해지면 설정만 올린다.
	 */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Party", meta = (ClampMin = "1"))
	int32 MinPlayersToStart = 1;

	UPROPERTY(Transient)
	FPrimaryAssetId PendingBattleDefinition;

private:
	/** 서버 트래블에 쓸 "맵경로?listen" URL을 만든다. 실패하면 빈 문자열을 반환한다. */
	FString BuildTravelUrl() const;
};
