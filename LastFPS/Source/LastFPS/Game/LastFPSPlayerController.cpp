#include "Game/LastFPSPlayerController.h"

#include "Character/LastFPSHero.h"
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
#include "UI/Framework/LastFPSPrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Input/CommonUIInputTypes.h"
#include "Engine/LocalPlayer.h"
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

ALastFPSPlayerController::ALastFPSPlayerController()
    : TeamId(FGenericTeamId::NoTeam)
{
}

void ALastFPSPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
    TeamId = NewTeamId;
}

FGenericTeamId ALastFPSPlayerController::GetGenericTeamId() const
{
    return TeamId;
}

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
    TryBindMenuLayerInputSync();

    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}

void ALastFPSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InitialScreenRetryTimerHandle);
        World->GetTimerManager().ClearTimer(HUDPushRetryTimerHandle);
        World->GetTimerManager().ClearTimer(MenuLayerSyncBindRetryTimerHandle);
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

// ── 커서/입력 config 단일 소유 ───────────────────────────────────────

void ALastFPSPlayerController::TryBindMenuLayerInputSync()
{
    if (bMenuLayerSyncBound)
    {
        return;
    }

    ULastFPSPrimaryGameLayout* Layout =
        Cast<ULastFPSPrimaryGameLayout>(UPrimaryGameLayout::GetPrimaryGameLayout(this));
    if (!Layout)
    {
        // 레이아웃(레이어 컨테이너) 준비 전 → 준비될 때까지 재시도.
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                MenuLayerSyncBindRetryTimerHandle, this,
                &ALastFPSPlayerController::TryBindMenuLayerInputSync, 0.1f, true);
        }
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MenuLayerSyncBindRetryTimerHandle);
    }

    // Modal/Menu/GameMenu 표시위젯 변경(전이 완료 시 발화)에 구독 → GetActiveWidget 확정 후라 타이밍 안전.
    for (int32 i = 0; i < ULastFPSPrimaryGameLayout::NumMenuLikeLayers; ++i)
    {
        if (UCommonActivatableWidgetContainerBase* Container = Layout->GetMenuLikeLayerContainer(i))
        {
            Container->OnDisplayedWidgetChanged().AddUObject(
                this, &ALastFPSPlayerController::HandleMenuLayerDisplayedWidgetChanged);
        }
    }

    bMenuLayerSyncBound = true;

    // 초기 상태 1회 정렬(허브 진입 직후 메뉴가 없으면 게임 config 로 맞춤).
    ApplyInputConfigForMenuState();
}

void ALastFPSPlayerController::HandleMenuLayerDisplayedWidgetChanged(UCommonActivatableWidget* /*NewWidget*/)
{
    ApplyInputConfigForMenuState();
}

void ALastFPSPlayerController::ApplyInputConfigForMenuState()
{
    if (!IsLocalController())
    {
        return;
    }

    ULastFPSPrimaryGameLayout* Layout =
        Cast<ULastFPSPrimaryGameLayout>(UPrimaryGameLayout::GetPrimaryGameLayout(this));
    if (!Layout)
    {
        return;
    }

    ULocalPlayer* LP = GetLocalPlayer();
    UCommonUIActionRouterBase* Router = LP ? LP->GetSubsystem<UCommonUIActionRouterBase>() : nullptr;
    if (!Router)
    {
        return;
    }

    // PC 가 커서/입력 config 의 유일한 writer(DefaultGame.ini 의 bEnableDefaultInputConfig=False 로
    // CommonUI 폴백 OFF). 위젯 config 에 의존하지 않고 "메뉴류가 있나"만 보고 양방향 확정 적용
    // → CommonUI 멀티루트/leaf 불확실성 우회. 라우터 API 로 내부 캐시까지 갱신해 재desync 방지.
    if (Layout->HasActiveMenuLikeWidget())
    {
        // 메뉴/모달 있음 → 커서 표시 + 게임입력 차단.
        Router->SetActiveUIInputConfig(
            FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture), this);
    }
    else
    {
        // 게임뷰만 → 커서 숨김 + 게임입력(마우스 캡처).
        Router->SetActiveUIInputConfig(
            FUIInputConfig(
                ECommonInputMode::Game,
                EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown,
                EMouseLockMode::LockOnCapture,
                /*bHideCursorDuringViewportCapture*/ true),
            this);
    }
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
    // NPC 상호작용 중에는 ESC로 ESC 메뉴를 열지 않는다.
    // (허브가 포커스를 잃으면 ESC가 위젯 대신 PC로 새서 ESC 메뉴가 뜨는 문제)
    // 대화창이 열려 있으면 대화창 Back에 맡기고, 아니면 허브를 닫아 상호작용 종료.
    if (InteractionSession.bActive)
    {
        if (!InteractionSession.bDialogueOpen)
        {
            if (ULastFPSNPCInteractionWidget* Hub = InteractionSession.HubWidget.Get())
            {
                Hub->DeactivateWidget(); // → NativeDestruct → EndNPCInteraction
            }
        }
        return;
    }

    if (EscMenuScreenTag.IsValid())
    {
        // 커서/입력 모드는 여기서 손대지 않는다. ESC 메뉴 화면은 ULastFPSActivatableWidget 계열이라
        // GetDesiredInputConfig()=Menu 가 활성화 시 자동 적용되고, 닫히면 CommonUI 가 스택 아래
        // (최종적으로 HUD 의 Game)로 되돌린다. PC 가 커서를 별도로 토글하면 이 단일 소스와 충돌해
        // "위젯 보이는데 커서 없음/위젯 없는데 커서 남음" desync 가 생긴다.
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

ULastFPSDialogueWidget* ALastFPSPlayerController::ShowDialogue(const FText& Speaker, const TArray<FText>& Lines)
{
    if (ULastFPSDialogueWidget* DialogueWidget = PushWidgetToModalLayer<ULastFPSDialogueWidget>(DialogueWidgetClass))
    {
        DialogueWidget->SetupDialogue(Speaker, Lines);
        return DialogueWidget;
    }
    return nullptr;
}

// ── NPC 상호작용 허브 (카메라 전환 + 액션 메뉴) ──────────────────────

void ALastFPSPlayerController::SetInteractionInputMode(bool bEnter)
{
    // 게임플레이 입력(이동/회전/사격/궁극기/ADS/스프린트) 차단만 이 함수가 담당한다.
    // 캐릭터의 매핑 컨텍스트를 통째로 제거/복원해 소스에서 막는다. (CommonUI Menu 모드는
    // 이 게임 매핑을 확실히 막지 못하므로 여기서 별도 처리 — CommonUI 커서 관리와 겹치지 않는 관심사.)
    //
    // 커서/입력 모드는 여기서 손대지 않는다. NPC 허브 위젯(ULastFPSActivatableWidget)의
    // GetDesiredInputConfig()=Menu 가 활성/파괴에 맞춰 CommonUI 를 통해 커서를 자동으로 켜고 끈다.
    // PC 가 SetInputMode/bShowMouseCursor 로 이중 제어하면 그 단일 소스와 충돌해 desync 가 난다.
    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetPawn()))
    {
        Hero->SetGameplayInputEnabled(!bEnter);
    }
}

void ALastFPSPlayerController::SetHubButtonsVisible(bool bVisible)
{
    if (ULastFPSNPCInteractionWidget* Hub = InteractionSession.HubWidget.Get())
    {
        Hub->SetButtonsVisible(bVisible);
    }
}

void ALastFPSPlayerController::BeginNPCInteraction(
    AActor* NPCActor,
    UCameraComponent* /*TalkCamera*/,
    const FText& Name,
    const FText& InRole,
    const TArray<FLastFPSNPCAction>& Actions)
{
    // 대상이 없거나 이미 상호작용 중이면 무시. (weak ptr이 아닌 bool로 판정 → 파괴/전환에도 안정)
    if (!NPCActor || InteractionSession.bActive)
    {
        return;
    }

    // 허브를 먼저 띄우고, 성공했을 때만 상태·입력·카메라를 적용한다.
    // (허브 생성 실패 시 카메라만 NPC에 고정되고 EndNPCInteraction이 영영 안 불리는 영구 고정 방지)
    UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(this);
    if (!RootLayout || !NPCInteractionWidgetClass)
    {
        UE_LOG(LogLastFPSPlayerController, Warning,
            TEXT("BeginNPCInteraction: 허브를 띄우지 못했습니다. PlayerController BP의 NPCInteractionWidgetClass에 WBP_NPCInteraction이 지정됐는지 확인하세요."));
        return;
    }

    ULastFPSNPCInteractionWidget* Hub = RootLayout->PushWidgetToLayerStack<ULastFPSNPCInteractionWidget>(
        LastFPSUITags::Layer_Menu(), NPCInteractionWidgetClass);
    if (!Hub)
    {
        UE_LOG(LogLastFPSPlayerController, Warning, TEXT("BeginNPCInteraction: 허브 위젯 push 실패."));
        return;
    }

    // ── 여기서부터 상호작용 확정 ──
    InteractionSession.bActive = true;
    InteractionSession.bDialogueOpen = false;
    InteractionSession.NPC = NPCActor;
    InteractionSession.NPCName = Name;
    InteractionSession.PreviousViewTarget = GetViewTarget();

    // 상호작용 UI 모드 진입: 이동/회전 잠금 + 커서 표시(단일 진입점).
    SetInteractionInputMode(true);

    // NPC 카메라로 블렌드. NPCActor의 UCameraComponent를 CalcCamera가 자동으로 뷰로 사용.
    SetViewTargetWithBlend(NPCActor, NPCCameraBlendTime);

    Hub->Setup(this, Name, InRole, Actions);
    InteractionSession.HubWidget = Hub;
}

void ALastFPSPlayerController::EndNPCInteraction()
{
    // 진입 여부는 bool 하나로 판정 → NPC/뷰타깃이 파괴돼도(레벨 전환 등) 입력은 반드시 복구된다.
    if (!InteractionSession.bActive)
    {
        return;
    }
    InteractionSession.bActive = false; // 아래 대화창 닫기 콜백의 재진입 방지

    // 열려 있던 대화창이 있으면 함께 닫는다 (허브만 닫히고 대화창이 남는 orphan 방지).
    if (ULastFPSDialogueWidget* Dialogue = InteractionSession.Dialogue.Get())
    {
        Dialogue->DeactivateWidget();
    }

    // 상호작용 UI 모드 종료: 이동/회전 잠금 해제 + 커서 숨김 (Begin과 대칭).
    SetInteractionInputMode(false);

    // 캐릭터(폰)로 시점 복귀.
    AActor* Target = InteractionSession.PreviousViewTarget.Get();
    if (!Target)
    {
        Target = GetPawn();
    }
    if (Target)
    {
        SetViewTargetWithBlend(Target, NPCCameraBlendTime);
    }

    InteractionSession.Reset();
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
        if (InteractionSession.bDialogueOpen)
        {
            break; // 이미 대화 중 → 중복 오픈(더블클릭) 방지.
        }

        const FLastFPSDialogueData* Dialogue = Action.DialogueRow.IsNull()
            ? nullptr
            : Action.DialogueRow.GetRow<FLastFPSDialogueData>(TEXT("NPC ExecuteNPCAction"));

        // Lines가 비면 SetupDialogue가 동기적으로 DeactivateWidget을 호출해 OnDeactivated가
        // 콜백 바인딩 전에 발화 → 버튼 영구 숨김/플래그 고착. 빈 대화는 아예 열지 않는다.
        if (Dialogue && Dialogue->Lines.Num() > 0)
        {
            // 행에 화자 이름이 없으면 현재 NPC 이름을 사용.
            const FText& Speaker = Dialogue->SpeakerName.IsEmpty() ? InteractionSession.NPCName : Dialogue->SpeakerName;

            // 대화창은 Modal 레이어라 Menu 레이어의 허브가 안 가려진다 → 허브 버튼을 수동으로 숨긴다.
            SetHubButtonsVisible(false);

            ULastFPSDialogueWidget* DialogueWidget = ShowDialogue(Speaker, Dialogue->Lines);
            if (DialogueWidget)
            {
                InteractionSession.bDialogueOpen = true;
                InteractionSession.Dialogue = DialogueWidget;

                // 대화 종료(DeactivateWidget) 시 버튼 복원 + 플래그 해제.
                // 바인드 대상=대화창, PC/허브는 weak 캡처 → 어느 쪽이 파괴돼도 안전.
                TWeakObjectPtr<ALastFPSPlayerController> WeakThis(this);
                TWeakObjectPtr<ULastFPSNPCInteractionWidget> HubWeak = InteractionSession.HubWidget;
                DialogueWidget->OnDeactivated().AddWeakLambda(DialogueWidget, [WeakThis, HubWeak]()
                {
                    if (ALastFPSPlayerController* PC = WeakThis.Get())
                    {
                        PC->InteractionSession.bDialogueOpen = false;
                        PC->InteractionSession.Dialogue.Reset();
                    }
                    if (ULastFPSNPCInteractionWidget* Hub = HubWeak.Get())
                    {
                        Hub->SetButtonsVisible(true);
                    }
                });
            }
            else
            {
                SetHubButtonsVisible(true); // 대화창을 못 띄웠으면 버튼 복원.
            }
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

void ALastFPSPlayerController::ShowDamageDirection(const FVector& DamageSourceDirection)
{
    if (HUDWidget)
    {
        HUDWidget->ShowDamageDirection(DamageSourceDirection);
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
