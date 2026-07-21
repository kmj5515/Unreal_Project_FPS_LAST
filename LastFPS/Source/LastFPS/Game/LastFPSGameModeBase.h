#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagContainer.h"
#include "LastFPSGameModeBase.generated.h"

class ALastFPSPlayerState;
class UAbilitySystemComponent;
class UGameplayEffect;
class ULastFPSCharacterDefinition;
class ULastFPSCharacterRoster;

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

    // 캐릭터 목록은 GameInstance의 로스터(단일 소스)에서 읽는다. (위젯·PC와 공유)
    const ULastFPSCharacterDefinition* GetCharacterDefinitionForIndex(int32 CharacterIndex) const;
    bool ApplyCharacterDefinitionToAbilitySystem(
        UAbilitySystemComponent* ASC,
        const ULastFPSCharacterDefinition* CharacterDefinition) const;

    // 서버: 이 맵의 제한 효과(예: 전투 금지)를 대상 ASC 에 적용한다.
    // "어떤 제한을 걸지"는 맵마다 다른 규칙이므로 InitialScreenTag 처럼 GameMode 가 소유한다.
    void ApplyLevelRestrictionsToAbilitySystem(UAbilitySystemComponent* ASC) const;

    // ── 이 맵의 UI 진입 설정 (PlayerController가 읽어 OpenScreen) ──
    // "어떤 화면을 띄울지"는 맵마다 다른 규칙이므로 GameMode가 소유한다.
    // 덕분에 PlayerController는 맵마다 다를 필요 없이 1개로 공유된다.

    FGameplayTag GetInitialScreenTag() const { return InitialScreenTag; }
    FGameplayTag GetEscMenuScreenTag() const { return EscMenuScreenTag; }
    bool ShouldShowQuestTracker() const { return bShowQuestTracker; }

    /** 맵 진입 시 자동으로 열 화면. 비우면 안 연다. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI", meta=(Categories="UI.Screen"))
    FGameplayTag InitialScreenTag;

    /** ESC로 열 화면 (예: 허브 메뉴). 비우면 ESC 무시. 닫기는 CommonUI Back. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI", meta=(Categories="UI.Screen"))
    FGameplayTag EscMenuScreenTag;

    /** 이 맵에서 상시 퀘스트 트래커 HUD를 표시할지. 전투 HUD와 독립 — 맵별로 GameMode BP에서 결정. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    bool bShowQuestTracker = false;

    /** 이 맵 진입 시 플레이어 ASC 에 적용할 제한 효과. 비우면 제한 없음(전투 맵). 허브 GameMode BP 에서 전투 금지 GE 지정. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Combat")
    TSubclassOf<UGameplayEffect> LevelRestrictionEffect;

protected:
    // 화면 디버그 + 로그를 한 번에 — 파생 GameMode들의 공용 헬퍼
    void DebugFlow(const FString& Message, FColor Color = FColor::Green) const;
};
