#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "GameplayTagContainer.h"
#include "Hub/ILastFPSInteractable.h"
#include "UI/LastFPSConfirmWidget.h"
#include "LastFPSPlayerController.generated.h"

class APawn;
class UCommonActivatableWidget;
class ULastFPSCharacterDefinition;
class ULastFPSCharacterRoster;
class ULastFPSHUDWidget;
class ULastFPSNoticeWidget;
class ULastFPSDialogueWidget;
class ULastFPSQuantityDialogWidget;

DECLARE_DYNAMIC_DELEGATE_OneParam(FLastFPSConfirmResultDelegate, bool, bConfirmed);
DECLARE_DYNAMIC_DELEGATE_OneParam(FLastFPSQuantityResultDelegate, int32, Quantity);

/**
 * 아웃게임 PlayerController.
 *
 * UI는 "얇은 리모컨" 역할만 한다 — OpenScreen/CloseScreen이 실제 로직을
 * ULastFPSUIManagerSubsystem에 위임한다. 화면별 push 메서드는 두지 않는다.
 * (화면 정의는 ScreenRegistry 데이터, 띄우는 로직은 Subsystem 한 곳)
 *
 * 책임: ① UI 진입점(façade) ② 캐릭터 선택 동기화 ③ NPC 상호작용 ④ 모달/공지
 */
UCLASS()
class LASTFPS_API ALastFPSPlayerController : public ACommonPlayerController
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupInputComponent() override;

    // ── UI 진입점 (Subsystem에 위임) ─────────────────────────────────

    /** 태그로 화면 열기. 실제 처리는 UIManagerSubsystem. */
    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    UCommonActivatableWidget* OpenScreen(FGameplayTag ScreenTag);

    /** 태그로 화면 닫기. */
    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    void CloseScreen(FGameplayTag ScreenTag);

    // ── 모달 · 공지 ──────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI", meta=(AutoCreateRefTerm="OnResult"))
    void ShowConfirm(const FText& Title, const FText& Message, FLastFPSConfirmResultDelegate OnResult);

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    void ShowNotice(const FText& Title, const FText& Message);

    /** 수량 선택 모달 표시 — 결과(선택 수량, 취소 시 0)는 OnResult 로 전달. */
    UFUNCTION(BlueprintCallable, Category="LastFPS|UI", meta=(AutoCreateRefTerm="OnResult"))
    void ShowQuantityPrompt(const FText& Title, const FText& ItemName, int32 UnitPrice, int32 MaxQuantity, FLastFPSQuantityResultDelegate OnResult);

    /** 단방향 NPC 대화창 표시. Lines를 "다음"으로 한 줄씩 진행. */
    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    void ShowDialogue(const FText& Speaker, const TArray<FText>& Lines);

    // ── 상호작용 (NPC) ──────────────────────────────────────────────

    /** NPC 범위 진입 시 호출 */
    void SetNearestInteractable(AActor* Interactable);
    /** NPC 범위 이탈 시 호출 */
    void ClearNearestInteractable(AActor* Interactable);

    // ── 캐릭터 선택 ─────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category="LastFPS|Character")
    void SetSelectedCharacterIndex(int32 NewIndex);

    UFUNCTION(BlueprintPure, Category="LastFPS|Character")
    int32 GetSelectedCharacterIndex() const { return SelectedCharacterIndex; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Character")
    TSubclassOf<APawn> GetSelectedCharacterClass() const;

    UFUNCTION(BlueprintPure, Category="LastFPS|Character")
    const ULastFPSCharacterDefinition* GetSelectedCharacterDefinition() const;

    /** 선택 가능한 캐릭터 정의 목록 — GameInstance의 로스터(단일 소스)에서 읽는다. */
    const TArray<TObjectPtr<ULastFPSCharacterDefinition>>& GetSelectableCharacterDefinitions() const;

    UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Character")
    void OnSelectedCharacterIndexChanged(int32 NewSelectedCharacterIndex);

    // ── HUD (인게임 팀 영역 — 휴면, 화면 라우팅과 별개) ──────────────

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    void ShowHitMarker();

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    ULastFPSHUDWidget* GetHUDWidget() const { return HUDWidget; }

protected:
    // 진입/ESC 화면 태그는 GameMode가 소유 → BeginPlay에 읽어와 캐시한다.
    // (맵별 차이를 GameMode가 가지므로 PlayerController는 1개로 공유 가능)

    // ── 모달 / HUD 위젯 클래스 ──────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSConfirmWidget> ConfirmWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSNoticeWidget> NoticeWidgetClass;

    /** 수량 선택 모달 위젯 클래스 (WBP_QuantityDialog) */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSQuantityDialogWidget> QuantityDialogWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSDialogueWidget> DialogueWidgetClass;

    /** 인게임 HUD (인게임 팀 영역, 현재 휴면) */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSHUDWidget> HUDWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    bool bPushHUDOnBeginPlay = false;

    // ── 내부 ────────────────────────────────────────────────────────

    /** GameMode에서 진입/ESC 화면 태그를 읽어 캐시 (BeginPlay) */
    void CacheUIConfigFromGameMode();

    /** 진입 화면 열기 — PrimaryGameLayout 준비될 때까지 재시도 */
    UFUNCTION()
    void OpenInitialScreen();

    /** ESC 입력 핸들러 — EscMenuScreenTag가 있으면 연다 */
    void HandleEscMenu();

    /** 인게임 HUD push (휴면). 레이아웃 준비 전이면 재시도. */
    void TryPushHUDToUILayout();

    template<typename TWidget>
    TWidget* PushWidgetToModalLayer(TSubclassOf<TWidget> WidgetClass);

    int32 ClampSelectedCharacterIndex(int32 NewIndex) const;
    void SyncSelectedCharacterState(int32 CharacterIndex);

    /** GameInstance의 캐릭터 로스터(단일 소스). 없으면 nullptr. */
    const ULastFPSCharacterRoster* GetCharacterRoster() const;

    UFUNCTION(Server, Reliable)
    void ServerSetSelectedCharacterIndex(int32 NewIndex);

    UFUNCTION()
    void OnRep_SelectedCharacterIndex();

    void TryInteract();

    UPROPERTY()
    TObjectPtr<ULastFPSHUDWidget> HUDWidget;

    UPROPERTY(ReplicatedUsing=OnRep_SelectedCharacterIndex, BlueprintReadOnly, Category="LastFPS|Character")
    int32 SelectedCharacterIndex = 0;

    FTimerHandle InitialScreenRetryTimerHandle;
    FTimerHandle HUDPushRetryTimerHandle;
    bool bHUDWidgetPushed = false;

    /** GameMode에서 읽어와 캐시한 진입/ESC 화면 태그 */
    FGameplayTag InitialScreenTag;
    FGameplayTag EscMenuScreenTag;

    /** 현재 범위 안에 있는 인터랙터블 (NPC 등) */
    TWeakObjectPtr<AActor> NearestInteractableActor;
};
