#include "Game/LastFPSPlayerController.h"

#include "Hub/ILastFPSInteractable.h"
#include "Data/Tables/LastFPSDialogueData.h"
#include "UI/HUD/LastFPSHUDWidget.h"
#include "UI/Common/LastFPSNoticeWidget.h"
#include "UI/Dialogue/LastFPSDialogueWidget.h"
#include "UI/Hub/LastFPSNPCInteractionWidget.h"
#include "UI/Common/LastFPSQuantityDialogWidget.h"

#include "Camera/CameraComponent.h"
#include "UI/Framework/LastFPSUIManagerSubsystem.h"
#include "UI/Framework/LastFPSUITags.h"

#include "CommonActivatableWidget.h"
#include "Engine/World.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Definitions/LastFPSCharacterRoster.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSGameModeBase.h"
#include "Game/LastFPSPlayerState.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"
#include "PrimaryGameLayout.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPlayerController, Log, All);

template<typename TWidget>
TWidget* ALastFPSPlayerController::PushWidgetToModalLayer(TSubclassOf<TWidget> WidgetClass)
{
    if (!WidgetClass)
    {
        return nullptr;
    }

    UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(this);
    if (!RootLayout)
    {
        return nullptr;
    }

    return RootLayout->PushWidgetToLayerStack<TWidget>(
        LastFPSUITags::Layer_Modal(),
        WidgetClass);
}

void ALastFPSPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSPlayerController, SelectedCharacterIndex);
}

void ALastFPSPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    if (bPushHUDOnBeginPlay)
    {
        TryPushHUDToUILayout();
    }

    CacheUIConfigFromGameMode();
    OpenInitialScreen();

    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}

void ALastFPSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InitialScreenRetryTimerHandle);
        World->GetTimerManager().ClearTimer(HUDPushRetryTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

// ── UI 진입점 (Subsystem에 위임) ─────────────────────────────────────

UCommonActivatableWidget* ALastFPSPlayerController::OpenScreen(FGameplayTag ScreenTag)
{
    if (ULastFPSUIManagerSubsystem* UI = ULastFPSUIManagerSubsystem::Get(this))
    {
        return UI->OpenScreen(ScreenTag, this);
    }
    return nullptr;
}

void ALastFPSPlayerController::CloseScreen(FGameplayTag ScreenTag)
{
    if (ULastFPSUIManagerSubsystem* UI = ULastFPSUIManagerSubsystem::Get(this))
    {
        UI->CloseScreen(ScreenTag);
    }
}

void ALastFPSPlayerController::CacheUIConfigFromGameMode()
{
    // "어떤 화면을 띄울지"는 맵 규칙이라 GameMode가 소유 → 여기서 읽어와 캐시.
    // (네트워크 클라이언트는 GetAuthGameMode가 null → 태그 비어 동작 안 함. standalone/호스트 OK.
    //  완전 네트워크 대응은 GameState 복제로 추후. TODO)
    if (const UWorld* World = GetWorld())
    {
        if (const ALastFPSGameModeBase* GM = World->GetAuthGameMode<ALastFPSGameModeBase>())
        {
            InitialScreenTag = GM->GetInitialScreenTag();
            EscMenuScreenTag = GM->GetEscMenuScreenTag();
        }
    }
}

void ALastFPSPlayerController::OpenInitialScreen()
{
    if (!InitialScreenTag.IsValid())
    {
        return;
    }

    // PrimaryGameLayout이 준비돼야 push 가능 → 준비될 때까지 재시도.
    if (UPrimaryGameLayout::GetPrimaryGameLayout(this))
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(InitialScreenRetryTimerHandle);
        }
        OpenScreen(InitialScreenTag);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            InitialScreenRetryTimerHandle, this,
            &ALastFPSPlayerController::OpenInitialScreen, 0.1f, true);
    }
}

// ── HUD (인게임 팀 영역 — 휴면) ─────────────────────────────────────

void ALastFPSPlayerController::TryPushHUDToUILayout()
{
    if (bHUDWidgetPushed || !HUDWidgetClass)
    {
        return;
    }

    UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(this);
    if (!RootLayout)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                HUDPushRetryTimerHandle,
                FTimerDelegate::CreateWeakLambda(this, [this]() { TryPushHUDToUILayout(); }),
                0.1f, true);
        }
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HUDPushRetryTimerHandle);
    }

    HUDWidget = RootLayout->PushWidgetToLayerStack<ULastFPSHUDWidget>(
        LastFPSUITags::Layer_Game(), HUDWidgetClass);
    bHUDWidgetPushed = (HUDWidget != nullptr);
}

// ── 입력 · 상호작용 ──────────────────────────────────────────────────

void ALastFPSPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (InputComponent)
    {
        // 상호작용은 G — F는 캐릭터 Enhanced Input의 궁극기(IA_Ultimate)와 충돌하므로.
        // 홀드 방식: 누르면 게이지 시작, 떼면 취소, 가득 차면 발동.
        InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ALastFPSPlayerController::BeginInteractHold);
        InputComponent->BindKey(EKeys::G, IE_Released, this, &ALastFPSPlayerController::EndInteractHold);

        // ESC → 설정된 메뉴 화면 열기. 열려 있을 땐 Menu 입력모드라 PC까지 안 오고
        // CommonUI Back이 받아 닫으므로 같은 키로 토글이 성립한다.
        InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ALastFPSPlayerController::HandleEscMenu);
    }
}

void ALastFPSPlayerController::HandleEscMenu()
{
    if (EscMenuScreenTag.IsValid())
    {
        OpenScreen(EscMenuScreenTag);
    }
}

void ALastFPSPlayerController::SetNearestInteractable(AActor* Interactable)
{
    NearestInteractableActor = Interactable;
}

void ALastFPSPlayerController::ClearNearestInteractable(AActor* Interactable)
{
    if (NearestInteractableActor.Get() == Interactable)
    {
        NearestInteractableActor.Reset();
    }

    // 홀드 중이던 대상이 범위를 벗어나면 취소.
    if (bIsInteractHeld && HeldInteractable.Get() == Interactable)
    {
        CancelInteractHold();
    }
}

// ── 홀드 인터랙션 ────────────────────────────────────────────────────

void ALastFPSPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (!bIsInteractHeld)
    {
        return;
    }

    AActor* Actor = HeldInteractable.Get();
    if (!Actor)
    {
        // 대상이 사라짐(파괴/범위 이탈) → 취소.
        CancelInteractHold();
        return;
    }

    InteractHoldElapsed += DeltaTime;
    const float Progress = (InteractHoldDuration > 0.f)
        ? FMath::Clamp(InteractHoldElapsed / InteractHoldDuration, 0.f, 1.f)
        : 1.f;

    UpdateInteractProgress(Progress);

    if (Progress >= 1.f)
    {
        CompleteInteractHold(Actor);
    }
}

void ALastFPSPlayerController::BeginInteractHold()
{
    AActor* Actor = NearestInteractableActor.Get();
    if (!Actor || !Actor->Implements<ULastFPSInteractable>())
    {
        return;
    }

    bIsInteractHeld = true;
    InteractHoldElapsed = 0.f;
    HeldInteractable = Actor;
    UpdateInteractProgress(0.f); // 게이지 0에서 시작
}

void ALastFPSPlayerController::EndInteractHold()
{
    // 가득 차기 전에 떼면 취소. (이미 완료됐으면 bIsInteractHeld=false라 무시)
    if (bIsInteractHeld)
    {
        CancelInteractHold();
    }
}

void ALastFPSPlayerController::CancelInteractHold()
{
    bIsInteractHeld = false;
    InteractHoldElapsed = 0.f;
    UpdateInteractProgress(0.f); // 게이지 리셋(숨김)
    HeldInteractable.Reset();
}

void ALastFPSPlayerController::CompleteInteractHold(AActor* Interactable)
{
    bIsInteractHeld = false;
    InteractHoldElapsed = 0.f;

    if (Interactable && Interactable->Implements<ULastFPSInteractable>())
    {
        ILastFPSInteractable::Execute_SetInteractionProgress(Interactable, 0.f); // 게이지 리셋
        ILastFPSInteractable::Execute_Interact(Interactable, this);
    }

    HeldInteractable.Reset();
}

void ALastFPSPlayerController::UpdateInteractProgress(float Progress)
{
    AActor* Actor = HeldInteractable.Get();
    if (Actor && Actor->Implements<ULastFPSInteractable>())
    {
        ILastFPSInteractable::Execute_SetInteractionProgress(Actor, Progress);
    }
}

// ── 모달 · 공지 ──────────────────────────────────────────────────────

void ALastFPSPlayerController::ShowConfirm(
    const FText& Title,
    const FText& Message,
    FLastFPSConfirmResultDelegate OnResult)
{
    if (ULastFPSConfirmWidget* ConfirmWidget = PushWidgetToModalLayer<ULastFPSConfirmWidget>(ConfirmWidgetClass))
    {
        ConfirmWidget->SetupConfirm(Title, Message);
        if (OnResult.IsBound())
        {
            ConfirmWidget->OnConfirmResult.Add(OnResult);
        }
    }
}

void ALastFPSPlayerController::ShowQuantityPrompt(
    const FText& Title,
    const FText& ItemName,
    int32 UnitPrice,
    int32 MaxQuantity,
    FLastFPSQuantityResultDelegate OnResult)
{
    if (ULastFPSQuantityDialogWidget* Dialog = PushWidgetToModalLayer<ULastFPSQuantityDialogWidget>(QuantityDialogWidgetClass))
    {
        Dialog->SetupQuantity(Title, ItemName, UnitPrice, MaxQuantity);
        if (OnResult.IsBound())
        {
            Dialog->OnQuantityResult.Add(OnResult);
        }
    }
    else
    {
        // 클래스 미지정 또는 레이아웃 없음 → 모달이 뜨지 않으니 원인을 로그로 남긴다
        UE_LOG(LogLastFPSPlayerController, Warning,
            TEXT("ShowQuantityPrompt: 수량 모달을 띄우지 못했습니다. PlayerController BP의 QuantityDialogWidgetClass에 WBP_QuantityDialog 가 지정됐는지 확인하세요."));
    }
}

void ALastFPSPlayerController::ShowNotice(const FText& Title, const FText& Message)
{
    if (ULastFPSNoticeWidget* NoticeWidget = PushWidgetToModalLayer<ULastFPSNoticeWidget>(NoticeWidgetClass))
    {
        NoticeWidget->SetupNotice(Title, Message);
    }
}

void ALastFPSPlayerController::ShowDialogue(const FText& Speaker, const TArray<FText>& Lines)
{
    if (ULastFPSDialogueWidget* DialogueWidget = PushWidgetToModalLayer<ULastFPSDialogueWidget>(DialogueWidgetClass))
    {
        DialogueWidget->SetupDialogue(Speaker, Lines);
    }
}

// ── NPC 상호작용 허브 (카메라 전환 + 액션 메뉴) ──────────────────────

void ALastFPSPlayerController::BeginNPCInteraction(
    AActor* NPCActor,
    UCameraComponent* /*TalkCamera*/,
    const FText& Name,
    const FText& InRole,
    const TArray<FLastFPSNPCAction>& Actions)
{
    // 대상이 없거나 이미 다른 NPC와 상호작용 중이면 무시.
    if (!NPCActor || CurrentNPC.IsValid())
    {
        return;
    }

    CurrentNPC = NPCActor;
    CurrentNPCName = Name;
    PreviousViewTarget = GetViewTarget();

    // NPC 카메라로 블렌드. NPCActor의 UCameraComponent를 CalcCamera가 자동으로 뷰로 사용.
    SetViewTargetWithBlend(NPCActor, NPCCameraBlendTime);

    // 허브 메뉴를 Menu 레이어에 push → 상점/모듈 화면이 그 위에 스택으로 열린다.
    UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(this);
    if (RootLayout && NPCInteractionWidgetClass)
    {
        ULastFPSNPCInteractionWidget* Hub = RootLayout->PushWidgetToLayerStack<ULastFPSNPCInteractionWidget>(
            LastFPSUITags::Layer_Menu(), NPCInteractionWidgetClass);
        if (Hub)
        {
            Hub->Setup(this, Name, InRole, Actions);
            NPCHubWidget = Hub;
        }
    }
    else
    {
        UE_LOG(LogLastFPSPlayerController, Warning,
            TEXT("BeginNPCInteraction: 허브를 띄우지 못했습니다. PlayerController BP의 NPCInteractionWidgetClass에 WBP_NPCInteraction이 지정됐는지 확인하세요."));
    }
}

void ALastFPSPlayerController::EndNPCInteraction()
{
    if (!CurrentNPC.IsValid() && !PreviousViewTarget.IsValid())
    {
        return; // 상호작용 중이 아님 (중복 호출 방지)
    }

    // 캐릭터(폰)로 시점 복귀. 이동은 허브가 닫히며 Game 입력모드로 자동 복구된다.
    AActor* Target = PreviousViewTarget.Get();
    if (!Target)
    {
        Target = GetPawn();
    }
    if (Target)
    {
        SetViewTargetWithBlend(Target, NPCCameraBlendTime);
    }

    CurrentNPC.Reset();
    PreviousViewTarget.Reset();
    NPCHubWidget.Reset();
    CurrentNPCName = FText::GetEmpty();
}

void ALastFPSPlayerController::ExecuteNPCAction(const FLastFPSNPCAction& Action)
{
    switch (Action.Type)
    {
    case ELastFPSNPCActionType::Screen:
        if (Action.ScreenTag.IsValid())
        {
            OpenScreen(Action.ScreenTag); // 상점/모듈/임무 등 — 허브 위에 스택으로 열림
        }
        break;

    case ELastFPSNPCActionType::Dialogue:
    {
        const FLastFPSDialogueData* Dialogue = Action.DialogueRow.IsNull()
            ? nullptr
            : Action.DialogueRow.GetRow<FLastFPSDialogueData>(TEXT("NPC ExecuteNPCAction"));
        if (Dialogue)
        {
            // 행에 화자 이름이 없으면 현재 NPC 이름을 사용.
            const FText& Speaker = Dialogue->SpeakerName.IsEmpty() ? CurrentNPCName : Dialogue->SpeakerName;
            ShowDialogue(Speaker, Dialogue->Lines); // Modal 레이어 → 허브 위에 표시
        }
        break;
    }
    }
}

void ALastFPSPlayerController::ShowHitMarker()
{
    if (HUDWidget)
    {
        HUDWidget->ShowHitMarker();
    }
}

// ── 캐릭터 선택 ──────────────────────────────────────────────────────

const ULastFPSCharacterRoster* ALastFPSPlayerController::GetCharacterRoster() const
{
    if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
    {
        return GI->GetCharacterRoster();
    }
    return nullptr;
}

const TArray<TObjectPtr<ULastFPSCharacterDefinition>>& ALastFPSPlayerController::GetSelectableCharacterDefinitions() const
{
    static const TArray<TObjectPtr<ULastFPSCharacterDefinition>> EmptyDefinitions;
    const ULastFPSCharacterRoster* Roster = GetCharacterRoster();
    return Roster ? Roster->Characters : EmptyDefinitions;
}

int32 ALastFPSPlayerController::ClampSelectedCharacterIndex(const int32 NewIndex) const
{
    const ULastFPSCharacterRoster* Roster = GetCharacterRoster();
    const int32 SelectableCount = Roster ? Roster->Num() : 0;
    const int32 MaxIndex = SelectableCount > 0 ? (SelectableCount - 1) : NewIndex;
    return FMath::Clamp(NewIndex, 0, MaxIndex);
}

void ALastFPSPlayerController::SyncSelectedCharacterState(const int32 CharacterIndex)
{
    if (ALastFPSPlayerState* LastPS = GetPlayerState<ALastFPSPlayerState>())
    {
        LastPS->Auth_SetSelectedCharacterIndex(CharacterIndex);

        if (ULastFPSGameInstance* LastGI = GetGameInstance<ULastFPSGameInstance>())
        {
            const FString PlayerKey = LastPS->GetPlayerName();
            LastGI->SaveSelectedCharacterIndex(PlayerKey, CharacterIndex);
        }
    }
}

void ALastFPSPlayerController::SetSelectedCharacterIndex(const int32 NewIndex)
{
    const int32 ClampedIndex = ClampSelectedCharacterIndex(NewIndex);
    SelectedCharacterIndex = ClampedIndex;
    ServerSetSelectedCharacterIndex(ClampedIndex);
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}

TSubclassOf<APawn> ALastFPSPlayerController::GetSelectedCharacterClass() const
{
    if (const ULastFPSCharacterDefinition* Definition = GetSelectedCharacterDefinition())
    {
        return Definition->PawnClass;
    }
    return nullptr;
}

const ULastFPSCharacterDefinition* ALastFPSPlayerController::GetSelectedCharacterDefinition() const
{
    const ULastFPSCharacterRoster* Roster = GetCharacterRoster();
    return Roster ? Roster->GetDefinition(SelectedCharacterIndex) : nullptr;
}

void ALastFPSPlayerController::ServerSetSelectedCharacterIndex_Implementation(const int32 NewIndex)
{
    const int32 ClampedIndex = ClampSelectedCharacterIndex(NewIndex);

    SelectedCharacterIndex = ClampedIndex;
    SyncSelectedCharacterState(ClampedIndex);
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}

void ALastFPSPlayerController::OnRep_SelectedCharacterIndex()
{
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}
