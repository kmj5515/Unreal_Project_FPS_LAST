#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTagContainer.h"
#include "Hub/ILastFPSInteractable.h"
#include "Hub/LastFPSNPCTypes.h"
#include "Quest/LastFPSNPCQuestOption.h"
#include "Data/Tables/LastFPSEquipmentStatTypes.h"
#include "UI/Result/LastFPSMissionResultTypes.h"
#include "LastFPSPlayerController.generated.h"

class APawn;
class UCameraComponent;
class UCommonActivatableWidget;
class ULastFPSCharacterDefinition;
class ULastFPSCharacterRoster;
class ULastFPSHUDWidget;
class ULastFPSDialogueWidget;
class ULastFPSEquipmentSubsystem;
class ULastFPSNPCInteractionWidget;
class UInputAction;
struct FInputActionInstance;

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
class LASTFPS_API ALastFPSPlayerController : public ACommonPlayerController, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    ALastFPSPlayerController();

    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
    virtual FGenericTeamId GetGenericTeamId() const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetPawn(APawn* InPawn) override;
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

    /** 공지 팝업이 완전히 비활성화된 뒤 후속 작업을 실행한다. */
    void ShowNoticeAfterClosed(const FText& Title, const FText& Message, FSimpleDelegate OnClosed);

    /**
     * 임무 결과 화면 표시. 전투 통계·사용 캐릭터는 이 컨트롤러가 PlayerState 에서 채우므로
     * 호출부는 임무 고유 정보(이름·시간·보상)만 넘긴다.
     */
    void ShowMissionResult(const FLastFPSMissionResult& InResult);

    /** 임무 결과 팝업이 완전히 비활성화된 뒤 후속 작업을 실행한다. */
    void ShowMissionResultAfterClosed(const FLastFPSMissionResult& InResult, FSimpleDelegate OnClosed);

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

    /** 퀘스트 용건이 있는 NPC의 상호작용 시작 — NPC 카메라로 블렌드 + 허브 메뉴 열기 */
    void BeginNPCInteraction(
        AActor* NPCActor,
        UCameraComponent* TalkCamera,
        const FText& Name,
        const FText& InDescription,
        const TArray<FLastFPSNPCAction>& Actions,
        const TArray<FLastFPSNPCQuestOption>& QuestOptions,
        const FLinearColor& InDialogueRadioSpeakerColor);

    /** NPC 상호작용 종료 — 캐릭터 카메라로 복귀 (허브 위젯 닫힐 때 호출) */
    void EndNPCInteraction();

    /** 허브 버튼 클릭 → 대화/화면 실행 (허브 위젯이 호출) */
    void ExecuteNPCAction(const FLastFPSNPCAction& Action);

	/** 퀘스트 내용 행 클릭 → 현재 NPC와 상태를 다시 검증한 뒤 수락 또는 보고한다. */
	bool ExecuteNPCQuestOption(const FLastFPSNPCQuestOption& Option);

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
    const TArray<TSoftObjectPtr<ULastFPSCharacterDefinition>>& GetSelectableCharacterDefinitions() const;

    UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Character")
    void OnSelectedCharacterIndexChanged(int32 NewSelectedCharacterIndex);

    /** 월드 기준 공격 방향을 현재 HUD의 화면 방향 인디케이터로 전달한다. */
    void ShowDamageDirection(const FVector& DamageSourceDirection);

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    ULastFPSHUDWidget* GetHUDWidget() const { return HUDWidget; }

    /** 게임 메뉴에서 호출하는 종료 확인 진입점. 실제 종료는 확인 이후에만 수행한다. */
    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    void RequestQuitGame();

    /** 개발용 콘솔 명령을 서버 권한의 인카운터 처리로 전달한다. Shipping 빌드에서는 동작하지 않는다. */
    void RequestDebugClearEncounter(FName EncounterId);

    /** 전투 서버가 각 소유 클라이언트를 로컬 허브로 복귀시킨다. */
    UFUNCTION(Client, Reliable)
    void ClientReturnToHub();

    /**
     * 클라이언트가 로컬로만 판정 가능한 파티 공용 목표(도달·획득)의 진행을 서버에 보고한다.
     * 서버는 파티 공용 퀘스트인지 다시 확인한 뒤 최댓값으로만 반영하므로, 잘못된 값이 와도
     * 진행이 되돌아가거나 요구량을 넘지 않는다.
     */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ReportPartyQuestProgress(FName QuestId, int32 ObjectiveIndex, int32 Progress);

protected:
    // 진입/ESC 화면 태그는 GameMode가 소유 → BeginPlay에 읽어와 캐시한다.
    // (맵별 차이를 GameMode가 가지므로 PlayerController는 1개로 공유 가능)

    // ── 모달 / HUD 위젯 클래스 ──────────────────────────────────────

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

    /**
     * 화면 단축키 — 액션 하나가 화면 태그 하나를 연다. 행을 추가하는 것이 곧 단축키 추가다.
     *
     * 실제 키는 IMC_Default 가 정한다. 여기서 키를 직접 들지 않는 이유는, 키 배정이 바뀌어도
     * C++ 과 PC 기본값이 그대로여야 하고 나중에 키 리바인딩 UI 를 붙일 여지를 남기기 위해서다.
     *
     * IMC_Default 는 NPC 상호작용 중 제거되므로 그동안 단축키도 함께 죽는다(의도된 동작).
     * 메뉴가 열린 뒤에는 입력 모드가 Menu 라 게임 입력이 오지 않는다. 닫기는 ESC 가 담당한다.
     */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI", meta=(Categories="UI.Screen"))
    TMap<TObjectPtr<const UInputAction>, FGameplayTag> ScreenHotkeys;

    /** NPC 상호작용 중에는 단축키로 화면을 열지 않는다(대화·상점 중 난입 방지). */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    bool bBlockScreenHotkeysDuringInteraction = true;

    // ── 내부 ────────────────────────────────────────────────────────

    /** GameMode에서 진입/ESC 화면 태그를 읽어 캐시 (BeginPlay) */
    /** 복제된 맵 UI 규칙을 캐시한다. 아직 도착하지 않았으면 false. */
    bool CacheUIConfigFromGameState();
    void ScheduleInitialScreenRetry();
    void ClearInitialScreenRetry();

    /** 진입 화면 열기 — PrimaryGameLayout 준비될 때까지 재시도 */
    UFUNCTION()
    void OpenInitialScreen();

    /** ESC 입력 핸들러 — EscMenuScreenTag가 있으면 연다. 커서/입력 모드는 CommonUI 입력설정 스택이 관리 */
    void HandleEscMenu();

    /**
     * 단축키 입력 핸들러 — 발동한 액션으로 ScreenHotkeys 를 조회해 해당 화면을 연다.
     * 액션마다 핸들러를 만들지 않으려고 인스턴스에서 소스 액션을 되읽는다.
     */
    void HandleScreenHotkey(const FInputActionInstance& ActionInstance);
    /** 인게임 HUD push (휴면). 레이아웃 준비 전이면 재시도. */
    void TryPushHUDToUILayout();

    /** 로컬 Pawn의 소유 및 BeginPlay 완료를 GameInstance의 로딩 흐름에 알린다. */
    void TryNotifyLocalPawnReady();

    // ── 커서/입력 config 단일 소유 ──────────────────────────────────
    // "메뉴류가 있으면 커서 표시, 없으면 숨김" 불변식을 PC 가 단독 소유.
    // Modal/Menu/GameMenu 표시위젯 변경 이벤트에 구독해 전이마다 재적용한다.

    /** 레이아웃 준비되면 메뉴 레이어 이벤트에 1회 구독. 준비 전이면 재시도. */
    void TryBindMenuLayerInputSync();

    /** 메뉴 레이어 표시위젯이 바뀔 때 → 입력 config 재평가. */
    void HandleMenuLayerDisplayedWidgetChanged(UCommonActivatableWidget* NewWidget);

    /** 메뉴류가 있으면 Menu config, 없으면 Game config 를 적용. */
    void ApplyInputConfigForMenuState();

    int32 ClampSelectedCharacterIndex(int32 NewIndex) const;
    void SyncSelectedCharacterState(int32 CharacterIndex);

    /** GameInstance의 캐릭터 로스터(단일 소스). 없으면 nullptr. */
    const ULastFPSCharacterRoster* GetCharacterRoster() const;

    UFUNCTION(Server, Reliable)
    void ServerSetSelectedCharacterIndex(int32 NewIndex);

    UFUNCTION(Server, Reliable)
    void ServerDebugClearEncounter(FName EncounterId);

    UFUNCTION()
    void OnRep_SelectedCharacterIndex();

    // ── 장비 구성 서버 제출 ──────────────────────────────────────────
    // 장비는 GameInstance 서브시스템(= 로컬 플레이어 소유)에만 있어 서버가 직접 읽으면
    // 리슨 서버에서는 호스트 장비가, 데디케이티드에서는 빈 장비가 전원에게 적용된다.
    // 소유 클라이언트가 아이템 행 ID 만 제출하고, 스탯은 서버가 데이터 테이블에서 다시 계산한다.

    ULastFPSEquipmentSubsystem* GetLocalEquipmentSubsystem() const;
    void BindEquipmentSubmission();
    void UnbindEquipmentSubmission();
    void SubmitEquipmentLoadoutToServer();

    UFUNCTION()
    void HandleLocalEquipmentChanged(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex);

    UFUNCTION(Server, Reliable)
    void Server_SubmitEquippedSlots(const TArray<FLastFPSEquippedSlot>& Slots);

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
    FTimerHandle MenuLayerSyncBindRetryTimerHandle;
    FTimerHandle LocalPawnReadyRetryTimerHandle;
    // 상호작용 종료 시 카메라 블렌드가 끝난 뒤 게임플레이 입력을 되돌리기 위한 타이머.
    FTimerHandle InteractionInputRestoreTimerHandle;
    bool bHUDWidgetPushed = false;
    bool bMenuLayerSyncBound = false;

    FGenericTeamId TeamId;

    /** GameMode에서 읽어와 캐시한 진입/ESC 화면 태그 */
    FGameplayTag InitialScreenTag;
    FGameplayTag EscMenuScreenTag;

    /** 현재 범위 안에 있는 인터랙터블 후보. 겹친 범위에서도 실제 거리를 다시 계산한다. */
    TSet<TWeakObjectPtr<AActor>> InteractableCandidates;

    /** 후보 중 현재 Pawn에 가장 가까운 인터랙터블 */
    TWeakObjectPtr<AActor> NearestInteractableActor;

	void RefreshNearestInteractable();

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
        FLinearColor DialogueRadioSpeakerColor = FLinearColor::White;
        TWeakObjectPtr<ULastFPSNPCInteractionWidget> HubWidget;
        TWeakObjectPtr<ULastFPSDialogueWidget> Dialogue; // orphan 방지용
        FRotator PreviousControlRotation = FRotator::ZeroRotator; // 진입 전 시선(복귀용)

        void Reset()
        {
            bActive = false;
            bDialogueOpen = false;
            NPC.Reset();
            PreviousViewTarget.Reset();
            NPCName = FText::GetEmpty();
            DialogueRadioSpeakerColor = FLinearColor::White;
            HubWidget.Reset();
            Dialogue.Reset();
            PreviousControlRotation = FRotator::ZeroRotator;
        }
    };
    FNPCInteractionSession InteractionSession;

    /**
     * NPC 상호작용 진입/종료 시 게임플레이 입력(이동/회전/사격 등) 매핑만 켜고/끈다.
     * 커서/입력 모드는 CommonUI 입력설정 스택(허브 위젯 GetDesiredInputConfig)이 단독 관리 — 여기서 안 만짐.
     */
    void SetInteractionInputMode(bool bEnter);
    /** 세션 허브의 액션 버튼 표시/숨김. */
    void SetHubButtonsVisible(bool bVisible);
};
