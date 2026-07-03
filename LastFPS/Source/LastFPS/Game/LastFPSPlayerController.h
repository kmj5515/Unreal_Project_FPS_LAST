#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "GameplayTagContainer.h"
#include "Hub/ILastFPSInteractable.h"
#include "Hub/LastFPSNPCTypes.h"
#include "UI/Common/LastFPSConfirmWidget.h"
#include "LastFPSPlayerController.generated.h"

class APawn;
class UCameraComponent;
class UCommonActivatableWidget;
class ULastFPSCharacterDefinition;
class ULastFPSCharacterRoster;
class ULastFPSHUDWidget;
class ULastFPSNoticeWidget;
class ULastFPSDialogueWidget;
class ULastFPSNPCInteractionWidget;
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
    virtual void PlayerTick(float DeltaTime) override;

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

    /** 단방향 NPC 대화창 표시. 생성된 위젯 반환(실패 시 null). */
    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    ULastFPSDialogueWidget* ShowDialogue(const FText& Speaker, const TArray<FText>& Lines);

    // ── 상호작용 (NPC) ──────────────────────────────────────────────

    /** NPC 범위 진입 시 호출 */
    void SetNearestInteractable(AActor* Interactable);
    /** NPC 범위 이탈 시 호출 */
    void ClearNearestInteractable(AActor* Interactable);

    // ── NPC 상호작용 허브 (카메라 전환 + 액션 메뉴) ──────────────────

    /** NPC 상호작용 시작 — NPC 카메라로 블렌드 + 허브 메뉴 열기 (InteractionComponent가 호출) */
    void BeginNPCInteraction(AActor* NPCActor, UCameraComponent* TalkCamera, const FText& Name, const FText& InRole, const TArray<FLastFPSNPCAction>& Actions);

    /** NPC 상호작용 종료 — 캐릭터 카메라로 복귀 (허브 위젯 닫힐 때 호출) */
    void EndNPCInteraction();

    /** 허브 버튼 클릭 → 대화/화면 실행 (허브 위젯이 호출) */
    void ExecuteNPCAction(const FLastFPSNPCAction& Action);

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

    /** NPC 상호작용 허브 위젯 클래스 (WBP_NPCInteraction) */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSNPCInteractionWidget> NPCInteractionWidgetClass;

    /** NPC 카메라 전환 블렌드 시간(초) */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Interaction", meta=(ClampMin="0"))
    float NPCCameraBlendTime = 0.4f;

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

    /**
     * ESC 메뉴 여닫을 때 커서/입력 모드를 명시적으로 강제한다.
     * ESC 메뉴는 NPC 상호작용(SetInteractionInputMode) 경로를 안 거치므로, 모달을 낀 상점 흐름
     * 뒤 굳어버린 마우스 캡처/커서 상태를 여기서 덮어써 커서가 반드시 보이도록 한다.
     * bMenuOpen=true → 커서 표시 + GameAndUI, false → 커서 숨김 + GameOnly.
     */
    void SetEscMenuInputMode(bool bMenuOpen);

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

    // ── 홀드 인터랙션 ────────────────────────────────────────────────
    // G를 누르고 있는 동안 게이지가 차오르고(InteractHoldDuration), 가득 차면 발동.
    // 도중에 떼거나 대상이 범위를 벗어나면 취소·리셋.

    /** G 눌림 — 홀드 시작 */
    void BeginInteractHold();
    /** G 뗌 — 홀드 취소 */
    void EndInteractHold();
    /** 홀드 중단(취소) + 게이지 리셋 */
    void CancelInteractHold();
    /** 게이지가 가득 참 → 실제 상호작용 발동 */
    void CompleteInteractHold(AActor* Interactable);
    /** 현재 대상 마커 게이지에 진행도(0~1) 반영 */
    void UpdateInteractProgress(float Progress);

    /** 홀드 발동까지 필요한 시간(초) */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Interaction", meta=(ClampMin="0.05"))
    float InteractHoldDuration = 1.0f;

    bool bIsInteractHeld = false;
    float InteractHoldElapsed = 0.f;
    /** 홀드 중인 대상 (홀드 동안 고정 — 도중 null 되면 취소) */
    TWeakObjectPtr<AActor> HeldInteractable;

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

    // ── NPC 상호작용 세션 (PC가 소유 — 복구 불변식 보존) ─────────────
    /**
     * 한 번의 NPC 상호작용 동안의 상태 묶음. 별도 UObject/Component로 빼지 않고
     * PC(가장 오래 사는 객체)가 소유 → 입력/카메라 "반드시 복구" 불변식을 지킨다.
     */
    struct FNPCInteractionSession
    {
        bool bActive = false;                            // 진입/입력잠금의 단일 진리원
        bool bDialogueOpen = false;                      // 대화창 열림(중복 오픈 방지)
        TWeakObjectPtr<AActor> NPC;                      // 현재 상호작용 NPC
        TWeakObjectPtr<AActor> PreviousViewTarget;       // 진입 전 시점(복귀용)
        FText NPCName;                                   // 대화 화자 폴백용
        TWeakObjectPtr<ULastFPSNPCInteractionWidget> HubWidget;
        TWeakObjectPtr<ULastFPSDialogueWidget> Dialogue; // orphan 방지용

        void Reset()
        {
            bActive = false;
            bDialogueOpen = false;
            NPC.Reset();
            PreviousViewTarget.Reset();
            NPCName = FText::GetEmpty();
            HubWidget.Reset();
            Dialogue.Reset();
        }
    };
    FNPCInteractionSession InteractionSession;

    /**
     * 상호작용 UI 모드 진입/종료를 대칭으로 토글하는 단일 진입점.
     * 이동/회전 입력 잠금 + 마우스 커서 표시를 한 곳에서 켜고/끈다(반쪽 수정·이중 제어 방지).
     */
    void SetInteractionInputMode(bool bEnter);
    /** 세션 허브의 액션 버튼 표시/숨김. */
    void SetHubButtonsVisible(bool bVisible);
};
