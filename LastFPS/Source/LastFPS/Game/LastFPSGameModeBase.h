#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "LastFPSGameModeBase.generated.h"

class ALastFPSPlayerState;
class UAbilitySystemComponent;
class UGameplayEffect;
class ULastFPSCharacterDefinition;
class ULastFPSCharacterRoster;
class ULastFPSBattleDefinition;
class ULastFPSDestinationContentComponent;
class ULastFPSDestinationContentSet;

UCLASS()
class LASTFPS_API ALastFPSGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALastFPSGameModeBase();

    virtual void InitGame(
        const FString& MapName,
        const FString& Options,
        FString& ErrorMessage) override;
    virtual void InitGameState() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    /**
     * 콘텐츠가 준비되기 전에는 스폰하지 않는다.
     * GetDefaultPawnClassForController 가 PawnClass 를 즉시 요구하므로,
     * 준비 전에 스폰하면 그 자리에서 동기 로드가 발생한다.
     */
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Players")
    int32 GetTotalConnectedPlayers() const;

    // 캐릭터 목록은 GameInstance의 로스터(단일 소스)에서 읽는다. (위젯·PC와 공유)
    const ULastFPSCharacterDefinition* GetCharacterDefinitionForIndex(int32 CharacterIndex) const;
    bool ApplyCharacterDefinitionToAbilitySystem(
        UAbilitySystemComponent* ASC,
        const ULastFPSCharacterDefinition* CharacterDefinition) const;

    /**
     * 미션 실패 처리 — 실패 이후 무엇을 할지는 맵마다 다른 규칙이므로 GameMode 가 소유한다
     * (InitialScreenTag / LevelRestrictionEffect 와 같은 소유 규칙).
     * 인카운터·목표 시스템은 "실패했다"만 알리고 이 결정에 관여하지 않는다.
     */
    UFUNCTION(BlueprintCallable, Category="LastFPS|Mission")
    void HandleMissionFailed(FName FailedEncounterId);

    // 서버: 이 맵의 제한 효과(예: 전투 금지)를 대상 ASC 에 적용한다.
    // "어떤 제한을 걸지"는 맵마다 다른 규칙이므로 InitialScreenTag 처럼 GameMode 가 소유한다.
    void ApplyLevelRestrictionsToAbilitySystem(UAbilitySystemComponent* ASC) const;

    // ── 이 맵의 UI 진입 설정 (PlayerController가 읽어 OpenScreen) ──
    // "어떤 화면을 띄울지"는 맵마다 다른 규칙이므로 GameMode가 소유한다.
    // 덕분에 PlayerController는 맵마다 다를 필요 없이 1개로 공유된다.

    FGameplayTag GetInitialScreenTag() const { return InitialScreenTag; }
    FGameplayTag GetEscMenuScreenTag() const { return EscMenuScreenTag; }
    bool ShouldShowQuestTracker() const { return bShowQuestTracker; }
    bool ShouldShowCombatHUD() const { return bShowCombatHUD; }
    ULastFPSDestinationContentSet* GetDestinationContentSet()
    {
        return DestinationContentSet;
    }
    const ULastFPSDestinationContentSet* GetDestinationContentSet() const
    {
        return DestinationContentSet;
    }

    /** 맵 진입 시 자동으로 열 화면. 비우면 안 연다. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI", meta=(Categories="UI.Screen"))
    FGameplayTag InitialScreenTag;

    /** ESC로 열 화면 (예: 허브 메뉴). 비우면 ESC 무시. 닫기는 CommonUI Back. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI", meta=(Categories="UI.Screen"))
    FGameplayTag EscMenuScreenTag;

    /** 이 맵에서 상시 퀘스트 트래커 HUD를 표시할지. 전투 HUD와 독립 — 맵별로 GameMode BP에서 결정. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    bool bShowQuestTracker = false;

    /**
     * 이 맵에서 전투 HUD 하단 레이어(무기 슬롯·스킬·체력바)를 표시할지.
     *
     * 허브에서는 무기를 들지 않으므로 무기 슬롯이 비어 있고 스킬도 쓸 수 없다. 빈 칸만 남는 UI 를
     * 띄우면 장착 결과가 반영되지 않은 것처럼 보인다. 기본값은 끔이라 허브·로비는 설정 없이 숨겨진다.
     * bShowQuestTracker 와 같은 성격의 맵별 규칙이라 GameMode 가 소유한다.
     */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    bool bShowCombatHUD = false;

    /** 이 맵 진입 시 플레이어 ASC 에 적용할 제한 효과. 비우면 제한 없음(전투 맵). 허브 GameMode BP 에서 전투 금지 GE 지정. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Combat")
    TSubclassOf<UGameplayEffect> LevelRestrictionEffect;

    /**
     * 이 맵에서 플레이어에게 무기 로드아웃을 장착시킬지. 기본값은 끔이라
     * 허브·로비처럼 전투가 없는 맵은 아무 설정 없이 맨손을 유지한다. 전투 맵 GameMode BP 에서만 켠다.
     * "어떤 맵에서 무기를 드는가"는 InitialScreenTag / LevelRestrictionEffect 와 같은 성격의 맵별 규칙이므로 GameMode 가 소유한다.
     */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Combat")
    bool bEquipWeaponLoadout = false;

    bool ShouldEquipWeaponLoadout() const { return bEquipWeaponLoadout; }

    /** 미션 실패 시 띄울 화면. 비우면 화면 없이 곧바로 귀환한다. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Mission", meta=(Categories="UI.Screen"))
    FGameplayTag MissionFailedScreenTag;

    /** 실패 화면을 보여준 뒤 귀환까지 기다리는 시간(초). 0 이면 즉시 귀환한다. */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Mission", meta=(ClampMin="0.0", Units="s"))
    float MissionFailedReturnDelay = 3.f;

    /** 미션 실패 시 허브로 돌려보낼지. 끄면 실패를 알리기만 하고 맵에 머문다(개발용). */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Mission")
    bool bReturnToHubOnMissionFailed = true;

    /**
     * 이 맵 진입 전에 준비할 콘텐츠. 비우면 게이트 없이 기존처럼 즉시 스폰한다.
     * 에셋 내부가 전부 소프트 참조라 이 강한 참조는 목록 자체만 로드한다.
     */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Loading")
    TObjectPtr<ULastFPSDestinationContentSet> DestinationContentSet;

protected:
    // 화면 디버그 + 로그를 한 번에 — 파생 GameMode들의 공용 헬퍼
    void DebugFlow(const FString& Message, FColor Color = FColor::Green) const;

private:
    /** 게이트 컴포넌트가 없는 맵(GameState BP 재지정 등)에서는 게이트 없이 진행한다. */
    bool IsDestinationContentReady() const;
    void HandleDestinationAssetsLoaded();
    void HandleRenderWarmupCompleted();
    void HandleDestinationContentReady();
    void SetLocalWarmupInputBlocked(APlayerController* PlayerController, bool bBlocked);

    /** 인카운터 실패 통지를 구독한다. 월드 서브시스템이 준비된 뒤 호출한다. */
    void BindEncounterFailureEvents();
    void UnbindEncounterFailureEvents();

    UFUNCTION()
    void HandleEncounterFailed(FName EncounterId);

    /** 실패 연출 후 실제 귀환을 수행한다. */
    void ExecuteMissionFailedReturn();

    TWeakObjectPtr<ULastFPSDestinationContentComponent> DestinationContentComponent;
    FTimerHandle MissionFailedReturnTimerHandle;
    bool bMissionFailedHandled = false;
    TArray<TWeakObjectPtr<APlayerController>> WarmupInputBlockedControllers;
    FDelegateHandle AssetsLoadedHandle;
    FDelegateHandle ContentReadyHandle;

    /** 이동 URL로 선택된 정의를 전투 월드 수명 동안 유지한다. */
    UPROPERTY(Transient)
    TObjectPtr<ULastFPSBattleDefinition> ActiveBattleDefinition;
};
