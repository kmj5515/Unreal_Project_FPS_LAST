#include "Game/LastFPSPlayerController.h"
#include "Character/LastFPSHero.h"
#include "Hub/ILastFPSInteractable.h"
#include "Hub/LastFPSNPC.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "Engine/GameInstance.h"
#include "Data/Tables/LastFPSDialogueData.h"
#include "UI/HUD/LastFPSHUDWidget.h"
#include "UI/Dialogue/LastFPSDialogueWidget.h"
#include "UI/Hub/LastFPSNPCInteractionWidget.h"
#include "UI/Common/LastFPSQuantityDialogWidget.h"
#include "UI/Framework/LastFPSPopupSubsystem.h"
#include "UI/Framework/LastFPSPopupTags.h"
#include "UI/Framework/LastFPSUIManagerSubsystem.h"
#include "UI/Framework/LastFPSUITags.h"
#include "CommonActivatableWidget.h"
#include "UI/Framework/LastFPSPrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Messaging/CommonGameDialog.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Definitions/LastFPSCharacterRoster.h"
#include "Encounter/LastFPSRoomEncounterSubsystem.h"
#include "Game/Loading/LastFPSLoadingProcessSubsystem.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSGameStateBase.h"
#include "Game/LastFPSPlayerState.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "Utility/LastFPSTeamTypes.h"
#include "EnhancedInputComponent.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"
#include "PrimaryGameLayout.h"
#include "TimerManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Localization/LastFPSLocalization.h"
#include "UI/Common/LastFPSConfirmWidget.h"
#include "UI/Common/LastFPSNoticeWidget.h"
#include "UI/Result/LastFPSMissionResultWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPlayerController, Log, All);

namespace
{
	/**
	 * 풀에서 재사용되는 팝업에 과거 후속 작업이 남지 않도록 비활성화 콜백을 1회만 실행한다.
	 *
	 * 브로드캐스트 도중 자기 바인딩을 Remove 하면 안 된다. Remove 는 락과 무관하게 델리게이트
	 * 인스턴스를 즉시 파괴하는데, 그 인스턴스가 지금 실행 중인 이 람다이고 OnClosed 는 그 캡처다.
	 * 파괴 후 캡처를 읽으면 해제된 메모리를 보게 되어 콜백이 unbound 로 보이고 후속 작업이 통째로
	 * 유실된다(퀘스트 체인이 보상 팝업에서 멈추는 원인이었다).
	 * 그래서 바인딩은 남겨 두고 공유 플래그로만 1회 실행을 보장한다. 바인딩은 위젯이 죽을 때 정리된다.
	 */
	void BindOneShotOnDeactivated(
		UCommonActivatableWidget& Widget,
		UObject& WeakOwner,
		FSimpleDelegate OnClosed)
	{
		const TSharedRef<bool> bAlreadyFired = MakeShared<bool>(false);
		Widget.OnDeactivated().AddWeakLambda(
			&WeakOwner,
			[bAlreadyFired, OnClosed = MoveTemp(OnClosed)]()
			{
				if (*bAlreadyFired)
				{
					return;
				}
				*bAlreadyFired = true;

				OnClosed.ExecuteIfBound();
			});
	}
}

void ALastFPSPlayerController::ClientReturnToHub_Implementation()
{
	if (ULastFPSGameInstance* LastFPSGameInstance =
		GetGameInstance<ULastFPSGameInstance>())
	{
		LastFPSGameInstance->RequestTravelToHub();
	}
}

bool ALastFPSPlayerController::Server_ReportPartyQuestProgress_Validate(
	const FName QuestId,
	const int32 ObjectiveIndex,
	const int32 Progress)
{
	// 값의 상한은 서버가 목표 요구량으로 다시 클램프한다. 여기서는 형태만 거른다.
	return !QuestId.IsNone() && ObjectiveIndex >= 0 && Progress >= 0;
}

void ALastFPSPlayerController::Server_ReportPartyQuestProgress_Implementation(
	const FName QuestId,
	const int32 ObjectiveIndex,
	const int32 Progress)
{
	if (ULastFPSQuestSubsystem* Quest = ULastFPSQuestSubsystem::Get(this))
	{
		Quest->Auth_ReportPartyObjectiveProgress(QuestId, ObjectiveIndex, Progress);
	}
}

#if !UE_BUILD_SHIPPING
namespace
{
	FName ResolveDebugEncounterId(const FString& RawArgument)
	{
		FString EncounterId = RawArgument;
		EncounterId.TrimStartAndEndInline();
		if (EncounterId.IsEmpty())
		{
			return NAME_None;
		}

		if (!EncounterId.Contains(TEXT(".")))
		{
			EncounterId = FString::Printf(TEXT("Room.%s"), *EncounterId);
		}
		return FName(*EncounterId);
	}

	void HandleClearEncounterCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Args.Num() != 1)
		{
			UE_LOG(
				LogLastFPSPlayerController,
				Warning,
				TEXT("사용법: LastFPS.Encounter.Clear <Zone3|Zone2|Boss|EncounterId>"));
			return;
		}

		const FName EncounterId = ResolveDebugEncounterId(Args[0]);
		if (EncounterId.IsNone())
		{
			UE_LOG(
				LogLastFPSPlayerController,
				Warning,
				TEXT("인카운터 ID가 비어 있습니다."));
			return;
		}

		if (ALastFPSPlayerController* Controller =
			Cast<ALastFPSPlayerController>(World->GetFirstPlayerController()))
		{
			Controller->RequestDebugClearEncounter(EncounterId);
			return;
		}

		if (World->GetNetMode() != NM_Client)
		{
			if (ULastFPSRoomEncounterSubsystem* EncounterSubsystem =
				World->GetSubsystem<ULastFPSRoomEncounterSubsystem>())
			{
				EncounterSubsystem->DebugClearEncounter(EncounterId);
				return;
			}
		}

		UE_LOG(
			LogLastFPSPlayerController,
			Warning,
			TEXT("[%s] 인카운터 클리어 요청을 전달할 서버 경로를 찾지 못했습니다."),
			*EncounterId.ToString());
	}

	FAutoConsoleCommandWithWorldAndArgs ClearEncounterCommand(
		TEXT("LastFPS.Encounter.Clear"),
		TEXT("개발용 활성 인카운터 즉시 클리어. 예: LastFPS.Encounter.Clear Zone3"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleClearEncounterCommand));
}
#endif

ALastFPSPlayerController::ALastFPSPlayerController()
    // NoTeam 으로 두면 이 컨트롤러가 가해자인 모든 피해가 "진영 미상"이 되어
    // 아군 판정을 통과하지 못한다. 즉 플레이어가 아군·펫·수호 목표까지 전부 때릴 수 있다.
    : TeamId(FGenericTeamId(static_cast<uint8>(ELastFPSTeam::Player)))
{
    // 액션 ↔ 화면 짝은 PC 블루프린트에서 저작한다. 생성자에서 에셋을 찾으면 C++ 이 콘텐츠 경로에 묶인다.
}

void ALastFPSPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
    TeamId = NewTeamId;
}

FGenericTeamId ALastFPSPlayerController::GetGenericTeamId() const
{
    return TeamId;
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

    // Party 맵(또는 이전 맵)에서 수동으로 설정한 UI Only 상태가 남아서 
    // 뷰포트의 인풋이 막히는 것을 방지하기 위해 초기화합니다.
    SetShowMouseCursor(false);
    SetInputMode(FInputModeGameOnly());

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (ULastFPSPopupSubsystem* PopupSubsystem = LocalPlayer
        ? LocalPlayer->GetSubsystem<ULastFPSPopupSubsystem>()
        : nullptr)
    {
        PopupSubsystem->BindDestinationContextSource(
            GetWorld() ? GetWorld()->GetGameState<ALastFPSGameStateBase>()
                       : nullptr);
    }

    if (bPushHUDOnBeginPlay)
    {
        TryPushHUDToUILayout();
    }

    // 태그 캐시는 OpenInitialScreen 이 매 시도마다 직접 수행한다(클라이언트는 GameState 복제가 늦다).
    OpenInitialScreen();
    TryBindMenuLayerInputSync();

    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);

    // 장비는 각 플레이어의 GameInstance 에만 있어 서버가 직접 읽을 수 없다.
    // 소유 클라이언트가 접속 직후 한 번, 이후 변경될 때마다 서버로 제출한다.
    BindEquipmentSubmission();
    SubmitEquipmentLoadoutToServer();

    UGameInstance* GameInstance = GetGameInstance();
    if (ULastFPSLoadingProcessSubsystem* LoadingProcesses = GameInstance
        ? GameInstance->GetSubsystem<ULastFPSLoadingProcessSubsystem>()
        : nullptr)
    {
        LoadingProcesses->NotifyLocalPlayerControllerReady(this);
    }
    TryNotifyLocalPawnReady();
}

void ALastFPSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindEquipmentSubmission();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InitialScreenRetryTimerHandle);
        World->GetTimerManager().ClearTimer(HUDPushRetryTimerHandle);
        World->GetTimerManager().ClearTimer(MenuLayerSyncBindRetryTimerHandle);
        World->GetTimerManager().ClearTimer(LocalPawnReadyRetryTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void ALastFPSPlayerController::SetPawn(APawn* InPawn)
{
    Super::SetPawn(InPawn);
    TryNotifyLocalPawnReady();
}

void ALastFPSPlayerController::TryNotifyLocalPawnReady()
{
    if (!IsLocalController())
    {
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    ULastFPSLoadingProcessSubsystem* LoadingProcesses = GameInstance
        ? GameInstance->GetSubsystem<ULastFPSLoadingProcessSubsystem>()
        : nullptr;
    if (!LoadingProcesses || !LoadingProcesses->IsWaitingForLocalPlayerPawn())
    {
        return;
    }

    APawn* CurrentPawn = GetPawn();
    if (!IsValid(CurrentPawn))
    {
        return;
    }

    if (CurrentPawn->GetController() == this && CurrentPawn->HasActorBegunPlay())
    {
        LoadingProcesses->NotifyLocalPlayerPawnReady(this);
        return;
    }

    if (UWorld* World = GetWorld();
        World && !World->GetTimerManager().IsTimerActive(LocalPawnReadyRetryTimerHandle))
    {
        LocalPawnReadyRetryTimerHandle = World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                TryNotifyLocalPawnReady();
            }));
    }
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

bool ALastFPSPlayerController::CacheUIConfigFromGameState()
{
    // "어떤 화면을 띄울지"는 맵 규칙이라 소유는 GameMode 에 두되, GameMode 는 서버에만 존재해
    // 클라이언트가 직접 읽을 수 없다. 값은 GameState 로 복제되므로 모든 머신이 여기서 읽는다.
    const UWorld* World = GetWorld();
    const ALastFPSGameStateBase* GameState =
        World ? World->GetGameState<ALastFPSGameStateBase>() : nullptr;
    if (!GameState || !GameState->HasMapUIRules())
    {
        return false;
    }

    const FLastFPSMapUIRules& Rules = GameState->GetMapUIRules();
    InitialScreenTag = Rules.InitialScreenTag;
    EscMenuScreenTag = Rules.EscMenuScreenTag;
    return true;
}

void ALastFPSPlayerController::ScheduleInitialScreenRetry()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            InitialScreenRetryTimerHandle, this,
            &ALastFPSPlayerController::OpenInitialScreen, 0.1f, true);
    }
}

void ALastFPSPlayerController::ClearInitialScreenRetry()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InitialScreenRetryTimerHandle);
    }
}

void ALastFPSPlayerController::OpenInitialScreen()
{
    // 클라이언트는 GameState 복제가 이 PlayerController 의 BeginPlay 보다 늦게 도착할 수 있다.
    // 예전에는 태그가 비면 재시도 없이 반환해, 클라이언트에서 초기 화면이 영영 열리지 않았다.
    if (!CacheUIConfigFromGameState())
    {
        ScheduleInitialScreenRetry();
        return;
    }

    // 규칙은 도착했는데 태그가 비어 있으면 이 맵은 초기 화면이 없다는 뜻이다. 재시도를 멈춘다.
    if (!InitialScreenTag.IsValid())
    {
        ClearInitialScreenRetry();
        return;
    }

    // PrimaryGameLayout이 준비돼야 push 가능 → 준비될 때까지 재시도.
    if (UPrimaryGameLayout::GetPrimaryGameLayout(this))
    {
        ClearInitialScreenRetry();
        OpenScreen(InitialScreenTag);
        return;
    }

    ScheduleInitialScreenRetry();
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

    // 화면 단축키는 Enhanced Input 으로 받는다. 실제 키는 IMC_Default 가 정하므로 여기엔 키가 없다.
    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    {
        for (const TPair<TObjectPtr<const UInputAction>, FGameplayTag>& Hotkey : ScreenHotkeys)
        {
            if (!Hotkey.Key || !Hotkey.Value.IsValid())
            {
                UE_LOG(LogLastFPSPlayerController, Warning,
                    TEXT("ScreenHotkeys 에 비어 있는 행이 있습니다(Action=%s, Tag=%s)."),
                    *GetNameSafe(Hotkey.Key), *Hotkey.Value.ToString());
                continue;
            }

            EnhancedInput->BindAction(
                Hotkey.Key, ETriggerEvent::Started, this, &ALastFPSPlayerController::HandleScreenHotkey);
        }
    }
}

void ALastFPSPlayerController::HandleScreenHotkey(const FInputActionInstance& ActionInstance)
{
    // 대화·상점 중에 다른 화면이 위로 뜨면 상호작용 종료 흐름이 꼬인다.
    if (bBlockScreenHotkeysDuringInteraction && InteractionSession.bActive)
    {
        return;
    }

    const FGameplayTag* ScreenTag = ScreenHotkeys.Find(ActionInstance.GetSourceAction());
    if (!ScreenTag || !ScreenTag->IsValid())
    {
        return;
    }

    // 커서/입력 모드는 건드리지 않는다. ESC 메뉴와 같은 이유로 CommonUI 단일 소스에 맡긴다.
    OpenScreen(*ScreenTag);
}

void ALastFPSPlayerController::RequestQuitGame()
{
    FLastFPSConfirmParams Params;
    Params.Title = FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::NoticeTitle);
    Params.Body = FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuitGameConfirmation);
    Params.OnResult = FCommonMessagingResultDelegate::CreateLambda([this](ECommonMessagingResult Result)
    {
        if (Result != ECommonMessagingResult::Confirmed) return;

        UKismetSystemLibrary::QuitGame(
            this, this
            , EQuitPreference::Quit, false);
    });

    ULastFPSConfirmWidget::ShowPopup(GetWorld(), Params);
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
	if (IsValid(Interactable))
	{
		InteractableCandidates.Add(TWeakObjectPtr<AActor>(Interactable));
	}
	RefreshNearestInteractable();
}

void ALastFPSPlayerController::ClearNearestInteractable(AActor* Interactable)
{
	InteractableCandidates.Remove(TWeakObjectPtr<AActor>(Interactable));
	RefreshNearestInteractable();

    // 홀드 중이던 대상이 범위를 벗어나면 취소.
    if (bIsInteractHeld && HeldInteractable.Get() == Interactable)
    {
        CancelInteractHold();
    }
}

void ALastFPSPlayerController::RefreshNearestInteractable()
{
	const APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
	{
		NearestInteractableActor.Reset();
		return;
	}

	AActor* Nearest = nullptr;
	double NearestDistanceSquared = TNumericLimits<double>::Max();
	for (auto It = InteractableCandidates.CreateIterator(); It; ++It)
	{
		AActor* Candidate = It->Get();
		if (!IsValid(Candidate))
		{
			It.RemoveCurrent();
			continue;
		}

		const double DistanceSquared = FVector::DistSquared(
			ControlledPawn->GetActorLocation(),
			Candidate->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			Nearest = Candidate;
		}
	}

	NearestInteractableActor = Nearest;
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
	RefreshNearestInteractable();
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
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    ULastFPSPopupSubsystem* PopupSubsystem = LocalPlayer
        ? LocalPlayer->GetSubsystem<ULastFPSPopupSubsystem>()
        : nullptr;
    if (!PopupSubsystem)
    {
        UE_LOG(
            LogLastFPSPlayerController,
            Warning,
            TEXT("ShowConfirm: 로컬 플레이어의 PopupSubsystem을 찾지 못했습니다."));
        OnResult.ExecuteIfBound(false);
        return;
    }

    FCommonMessagingResultDelegate ResultCallback =
        FCommonMessagingResultDelegate::CreateWeakLambda(
            this,
            [OnResult](const ECommonMessagingResult Result) mutable
            {
                OnResult.ExecuteIfBound(
                    Result == ECommonMessagingResult::Confirmed);
            });

    UCommonGameDialogDescriptor* Descriptor =
        UCommonGameDialogDescriptor::CreateConfirmationYesNo(Title, Message);
    PopupSubsystem->ShowConfirmation(Descriptor, MoveTemp(ResultCallback));
}

void ALastFPSPlayerController::ShowQuantityPrompt(
    const FText& Title,
    const FText& ItemName,
    int32 UnitPrice,
    int32 MaxQuantity,
    FLastFPSQuantityResultDelegate OnResult)
{
    ULastFPSQuantityDialogWidget* Dialog =
        ULastFPSPopupSubsystem::ShowPopup<ULastFPSQuantityDialogWidget>(
            this, LastFPSPopupTags::Quantity());
    if (Dialog)
    {
        Dialog->SetupQuantity(Title, ItemName, UnitPrice, MaxQuantity);

        if (OnResult.IsBound())
        {
            Dialog->OnQuantityResult.Add(OnResult);
        }
    }
    else
    {
        UE_LOG(
            LogLastFPSPlayerController,
            Warning,
            TEXT("ShowQuantityPrompt: 수량 팝업을 열지 못했습니다."));
        OnResult.ExecuteIfBound(0);
    }
}

void ALastFPSPlayerController::ShowNotice(const FText& Title, const FText& Message)
{
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (ULastFPSPopupSubsystem* PopupSubsystem = LocalPlayer
        ? LocalPlayer->GetSubsystem<ULastFPSPopupSubsystem>()
        : nullptr)
    {
        UCommonGameDialogDescriptor* Descriptor =
            UCommonGameDialogDescriptor::CreateConfirmationOk(Title, Message);
        PopupSubsystem->ShowError(Descriptor);
        return;
    }

    UE_LOG(
        LogLastFPSPlayerController,
        Warning,
        TEXT("ShowNotice: 로컬 플레이어의 PopupSubsystem을 찾지 못했습니다."));
}

void ALastFPSPlayerController::ShowNoticeAfterClosed(
    const FText& Title,
    const FText& Message,
    FSimpleDelegate OnClosed)
{
    FLastFPSNoticeParams Params;
    Params.Title = Title;
    Params.Body = Message;

    ULastFPSNoticeWidget* Notice = ULastFPSNoticeWidget::ShowPopup(this, Params);
    if (!Notice)
    {
        UE_LOG(
            LogLastFPSPlayerController,
            Warning,
            TEXT("ShowNoticeAfterClosed: 공지 팝업을 열지 못해 후속 작업을 즉시 실행합니다."));
        OnClosed.ExecuteIfBound();
        return;
    }

	BindOneShotOnDeactivated(*Notice, *this, MoveTemp(OnClosed));
}

void ALastFPSPlayerController::ShowMissionResult(const FLastFPSMissionResult& InResult)
{
	ShowMissionResultAfterClosed(InResult, FSimpleDelegate());
}

void ALastFPSPlayerController::ShowMissionResultAfterClosed(
	const FLastFPSMissionResult& InResult,
	FSimpleDelegate OnClosed)
{
    // 임무 고유 정보는 호출부가, 플레이어에 종속된 값은 여기서 채운다.
    // 이렇게 두면 결과를 만드는 쪽(퀘스트 등)이 PlayerState 나 로스터를 알지 않아도 된다.
    FLastFPSMissionResult Result = InResult;

    if (const ALastFPSPlayerState* LastFPSPlayerState = GetPlayerState<ALastFPSPlayerState>())
    {
        Result.CombatStats.DamageDealt = LastFPSPlayerState->GetStatDamageDealt();
        Result.CombatStats.DamageTaken = LastFPSPlayerState->GetStatDamageTaken();
        Result.CombatStats.Kills = LastFPSPlayerState->GetStatKills();
        Result.CombatStats.Deaths = LastFPSPlayerState->GetStatDeaths();
        Result.CombatStats.Assists = LastFPSPlayerState->GetStatAssists();
    }

    if (Result.CharacterPortrait.IsNull())
    {
        if (const ULastFPSCharacterRoster* Roster = GetCharacterRoster())
        {
            if (const ULastFPSCharacterDefinition* Definition =
                    Roster->GetDefinition(SelectedCharacterIndex))
            {
                Result.CharacterPortrait = Definition->Icon;
            }
        }
    }

	ULastFPSMissionResultWidget* ResultWidget =
		ULastFPSMissionResultWidget::ShowPopup(this, Result);
	if (!ResultWidget)
	{
		UE_LOG(
			LogLastFPSPlayerController,
			Warning,
			TEXT("ShowMissionResultAfterClosed: 임무 결과 팝업을 열지 못해 후속 작업을 즉시 실행합니다."));
		OnClosed.ExecuteIfBound();
		return;
	}

	BindOneShotOnDeactivated(*ResultWidget, *this, MoveTemp(OnClosed));
}

ULastFPSDialogueWidget* ALastFPSPlayerController::ShowDialogue(
    const FText& Speaker,
    const TArray<FText>& Lines)
{
    ULastFPSDialogueWidget* Dialogue =
        ULastFPSPopupSubsystem::ShowPopup<ULastFPSDialogueWidget>(
            this, LastFPSPopupTags::Dialogue());
    if (!Dialogue)
    {
        UE_LOG(
            LogLastFPSPlayerController,
            Warning,
            TEXT("ShowDialogue: 대화 팝업을 열지 못했습니다."));
        return nullptr;
    }

    Dialogue->SetupDialogue(Speaker, Lines);
    return Dialogue;
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
    const FText& InDescription,
    const TArray<FLastFPSNPCAction>& Actions,
    const TArray<FLastFPSNPCQuestOption>& QuestOptions,
    const FLinearColor& InDialogueRadioSpeakerColor)
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
    InteractionSession.DialogueRadioSpeakerColor = InDialogueRadioSpeakerColor;
    InteractionSession.PreviousViewTarget = GetViewTarget();

    // 직전 상호작용의 지연 복구가 남아 있으면 취소한다(연속 상호작용 시 입력이 뒤늦게 풀리는 것 방지).
    GetWorldTimerManager().ClearTimer(InteractionInputRestoreTimerHandle);

    // 종료 시 되돌릴 시선. 허브가 열린 동안 뷰타깃은 NPC라 컨트롤 회전 변화가 화면에 안 보이므로,
    // 한 프레임이라도 회전 입력이 새면 복귀 시 엉뚱한 각도에서 시작한다. 각도를 여기서 붙잡아 둔다.
    InteractionSession.PreviousControlRotation = GetControlRotation();

    // 상호작용 UI 모드 진입: 이동/회전 잠금 + 커서 표시(단일 진입점).
    SetInteractionInputMode(true);

    // NPC 카메라로 블렌드. NPCActor의 UCameraComponent를 CalcCamera가 자동으로 뷰로 사용.
    SetViewTargetWithBlend(NPCActor, NPCCameraBlendTime);

    Hub->Setup(this, Name, InDescription, Actions, QuestOptions);
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

    // 진입 전 시선으로 되돌린다. 허브 열림/닫힘 사이에 새어 들어온 회전을 여기서 무효화한다.
    SetControlRotation(InteractionSession.PreviousControlRotation);

    // 상호작용 UI 모드 종료: 이동/회전 잠금 해제 (Begin과 대칭).
    // 단, 카메라가 폰으로 블렌드되는 동안에는 입력을 열지 않는다. 블렌드 중 들어온 회전은
    // 화면에 보이지 않는 상태로 컨트롤 회전만 틀어놓아, 복귀 직후 시점이 튀는 원인이 된다.
    if (NPCCameraBlendTime > 0.f)
    {
        GetWorldTimerManager().SetTimer(
            InteractionInputRestoreTimerHandle,
            FTimerDelegate::CreateUObject(this, &ALastFPSPlayerController::SetInteractionInputMode, false),
            NPCCameraBlendTime,
            false);
    }
    else
    {
        SetInteractionInputMode(false);
    }

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
        const FLastFPSDialogueData* Dialogue = Action.DialogueRow.IsNull()
            ? nullptr
            : Action.DialogueRow.GetRow<FLastFPSDialogueData>(TEXT("NPC ExecuteNPCAction"));

		if (!Dialogue || Dialogue->Lines.IsEmpty())
		{
			UE_LOG(
				LogLastFPSPlayerController,
				Warning,
				TEXT("ExecuteNPCAction: Dialogue 액션의 행이 없거나 대사가 비어 있습니다. Row=%s"),
				*Action.DialogueRow.RowName.ToString());
			break;
		}

		if (ULastFPSQuestSubsystem* Quest = ULastFPSQuestSubsystem::Get(this))
		{
			// Dialogue Data는 유지하고, 표현 계층만 HUD 무전 WBP로 통일한다.
			Quest->TriggerDialogueAsRadio(
				*Dialogue,
				InteractionSession.NPCName,
				InteractionSession.DialogueRadioSpeakerColor);
		}
		else
		{
			UE_LOG(
				LogLastFPSPlayerController,
				Error,
				TEXT("ExecuteNPCAction: 무전 대화를 재생할 QuestSubsystem을 찾지 못했습니다. Row=%s"),
				*Action.DialogueRow.RowName.ToString());
		}
        break;
    }
    }
}

bool ALastFPSPlayerController::ExecuteNPCQuestOption(const FLastFPSNPCQuestOption& Option)
{
	const ALastFPSNPC* NPC = Cast<ALastFPSNPC>(InteractionSession.NPC.Get());
	ULastFPSQuestSubsystem* Quest = ULastFPSQuestSubsystem::Get(this);
	if (NPC == nullptr || Quest == nullptr || Option.QuestId.IsNone() || NPC->NPCRowName.IsNone())
	{
		return false;
	}

	// 화면이 열린 뒤 상태가 바뀌었을 수 있으므로 클릭 시점의 실제 가능 목록으로 다시 검증한다.
	TArray<FName> Acceptable;
	TArray<FName> Reportable;
	Quest->GetNPCQuestActions(NPC->NPCRowName, Acceptable, Reportable);

	switch (Option.Type)
	{
	case ELastFPSNPCQuestOptionType::Accept:
		if (!Acceptable.Contains(Option.QuestId) || !Quest->AcceptQuest(Option.QuestId))
		{
			return false;
		}

		// 의뢰인과 나눈 수락 대화가 같은 NPC를 대상으로 한 첫 TalkToNPC 목표라면
		// 별도의 두 번째 클릭을 요구하지 않고 이번 상호작용으로 바로 반영한다.
		Quest->NotifyTalkedToNPCForQuest(Option.QuestId, NPC->NPCRowName);
		return true;

	case ELastFPSNPCQuestOptionType::Report:
		if (!Reportable.Contains(Option.QuestId)
			|| !Quest->NotifyTalkedToNPCForQuest(Option.QuestId, NPC->NPCRowName))
		{
			return false;
		}

		// NPC에게 보고하는 클릭은 목표 완료와 보상 수령을 합친 명시적 턴인 동작이다.
		// 아직 다른 목표가 남았다면 InProgress이므로 수령하지 않고, 이번 보고로 Completed가 된 경우만 지급한다.
		if (Quest->IsClaimable(Option.QuestId))
		{
			return Quest->TryClaimReward(Option.QuestId);
		}
		return true;

	case ELastFPSNPCQuestOptionType::OpenGuidanceScreen:
	{
		TArray<FLastFPSNPCQuestOption> CurrentOptions;
		Quest->GetNPCQuestOptions(NPC->NPCRowName, CurrentOptions);
		const bool bStillAvailable = CurrentOptions.ContainsByPredicate(
			[&Option](const FLastFPSNPCQuestOption& Current)
			{
				return Current.Type == ELastFPSNPCQuestOptionType::OpenGuidanceScreen
					&& Current.QuestId == Option.QuestId
					&& Current.ScreenTag == Option.ScreenTag;
			});
		if (!bStillAvailable || !Option.ScreenTag.IsValid())
		{
			return false;
		}

		OpenScreen(Option.ScreenTag);
		return true;
	}
	}

	return false;
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

const TArray<TSoftObjectPtr<ULastFPSCharacterDefinition>>& ALastFPSPlayerController::GetSelectableCharacterDefinitions() const
{
    static const TArray<TSoftObjectPtr<ULastFPSCharacterDefinition>> EmptyDefinitions;
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
        return Definition->PawnClass.LoadSynchronous();
    }
    return nullptr;
}

const ULastFPSCharacterDefinition* ALastFPSPlayerController::GetSelectedCharacterDefinition() const
{
    const ULastFPSCharacterRoster* Roster = GetCharacterRoster();
    return Roster ? Roster->GetDefinition(SelectedCharacterIndex) : nullptr;
}

void ALastFPSPlayerController::RequestDebugClearEncounter(const FName EncounterId)
{
#if UE_BUILD_SHIPPING
    UE_LOG(
        LogLastFPSPlayerController,
        Warning,
        TEXT("Shipping 빌드에서는 인카운터 클리어 치트를 사용할 수 없습니다."));
#else
    if (EncounterId.IsNone())
    {
        UE_LOG(
            LogLastFPSPlayerController,
            Warning,
            TEXT("인카운터 클리어 치트에 유효한 EncounterId가 필요합니다."));
        return;
    }

    if (!HasAuthority())
    {
        ServerDebugClearEncounter(EncounterId);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        if (ULastFPSRoomEncounterSubsystem* EncounterSubsystem =
            World->GetSubsystem<ULastFPSRoomEncounterSubsystem>())
        {
            EncounterSubsystem->DebugClearEncounter(EncounterId);
        }
    }
#endif
}

void ALastFPSPlayerController::ServerDebugClearEncounter_Implementation(const FName EncounterId)
{
#if !UE_BUILD_SHIPPING
    if (UWorld* World = GetWorld())
    {
        if (ULastFPSRoomEncounterSubsystem* EncounterSubsystem =
            World->GetSubsystem<ULastFPSRoomEncounterSubsystem>())
        {
            EncounterSubsystem->DebugClearEncounter(EncounterId);
        }
    }
#endif
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

ULastFPSEquipmentSubsystem* ALastFPSPlayerController::GetLocalEquipmentSubsystem() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>() : nullptr;
}

void ALastFPSPlayerController::BindEquipmentSubmission()
{
    if (ULastFPSEquipmentSubsystem* Equipment = GetLocalEquipmentSubsystem())
    {
        Equipment->OnEquipmentChanged.AddUniqueDynamic(
            this, &ALastFPSPlayerController::HandleLocalEquipmentChanged);
    }
}

void ALastFPSPlayerController::UnbindEquipmentSubmission()
{
    if (ULastFPSEquipmentSubsystem* Equipment = GetLocalEquipmentSubsystem())
    {
        Equipment->OnEquipmentChanged.RemoveDynamic(
            this, &ALastFPSPlayerController::HandleLocalEquipmentChanged);
    }
}

void ALastFPSPlayerController::HandleLocalEquipmentChanged(
    ELastFPSEquipmentSlotType /*SlotType*/, int32 /*SlotIndex*/)
{
    SubmitEquipmentLoadoutToServer();
}

void ALastFPSPlayerController::SubmitEquipmentLoadoutToServer()
{
    if (!IsLocalController())
    {
        return;
    }

    if (const ULastFPSEquipmentSubsystem* Equipment = GetLocalEquipmentSubsystem())
    {
        Server_SubmitEquippedSlots(Equipment->BuildEquippedSlots());
    }
}

void ALastFPSPlayerController::Server_SubmitEquippedSlots_Implementation(
    const TArray<FLastFPSEquippedSlot>& Slots)
{
    if (ALastFPSPlayerState* LastPS = GetPlayerState<ALastFPSPlayerState>())
    {
        LastPS->Auth_SetEquippedSlots(Slots);
    }
}
