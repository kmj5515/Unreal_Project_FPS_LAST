#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagContainer.h"
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

    // ── 이 맵의 UI 진입 설정 (PlayerController가 읽어 OpenScreen) ──
    // "어떤 화면을 띄울지"는 맵마다 다른 규칙이므로 GameMode가 소유한다.
    // 덕분에 PlayerController는 맵마다 다를 필요 없이 1개로 공유된다.

    FGameplayTag GetInitialScreenTag() const { return InitialScreenTag; }
    FGameplayTag GetEscMenuScreenTag() const { return EscMenuScreenTag; }

    /** 맵 진입 시 자동으로 열 화면. 비우면 안 연다. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI", meta=(Categories="UI.Screen"))
    FGameplayTag InitialScreenTag;

    /** ESC로 열 화면 (예: 허브 메뉴). 비우면 ESC 무시. 닫기는 CommonUI Back. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI", meta=(Categories="UI.Screen"))
    FGameplayTag EscMenuScreenTag;

protected:
    // 화면 디버그 + 로그를 한 번에 — 파생 GameMode들의 공용 헬퍼
    void DebugFlow(const FString& Message, FColor Color = FColor::Green) const;
};
