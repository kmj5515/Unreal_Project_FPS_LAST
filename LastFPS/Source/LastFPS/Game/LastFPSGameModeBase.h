#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LastFPSGameModeBase.generated.h"

class ALastFPSPlayerState;

UCLASS()
class LASTFPS_API ALastFPSGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALastFPSGameModeBase();

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Players")
    int32 GetTotalConnectedPlayers() const;

    // 캐릭터 선택 인덱스별 Pawn 클래스 목록
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Character")
    TArray<TSubclassOf<APawn>> CharacterPawnClasses;

protected:
    // 화면 디버그 + 로그를 한 번에 — 파생 GameMode들의 공용 헬퍼
    void DebugFlow(const FString& Message, FColor Color = FColor::Green) const;
};
