#include "Game/LastFPSGameModeBase.h"
#include "AbilitySystemComponent.h"
#include "Data/Characters/LastFPSCharacterStatData.h"
#include "Data/Definitions/LastFPSActorPoolProfile.h"
#include "Data/Definitions/LastFPSBattleDefinition.h"
#include "Data/Definitions/LastFPSDestinationContentSet.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Definitions/LastFPSCharacterRoster.h"
#include "Encounter/LastFPSRoomEncounterSubsystem.h"
#include "Engine/World.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSGameStateBase.h"
#include "Game/LastFPSGameStateBase.h"
#include "Game/LastFPSPlayerController.h"
#include "Game/LastFPSPlayerState.h"
#include "Game/Loading/LastFPSDestinationContentComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSGameMode, Log, All);

ALastFPSGameModeBase::ALastFPSGameModeBase()
{
    // 플레이어 ASC(PlayerState 소유)·스탯·캐릭터 선택이 ALastFPSPlayerState 를 전제하므로 명시적으로 지정한다.
    // (미지정 시 AGameModeBase 기본 APlayerState 가 쓰여 GetPlayerState<ALastFPSPlayerState> 가 항상 null → ASC/스탯 소실.)
    PlayerStateClass = ALastFPSPlayerState::StaticClass();

    // 로딩 게이트 컴포넌트의 부착 지점. 같은 이유로 명시 지정한다.
    GameStateClass = ALastFPSGameStateBase::StaticClass();
}

void ALastFPSGameModeBase::InitGame(
    const FString& MapName,
    const FString& Options,
    FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    if (!ErrorMessage.IsEmpty())
    {
        return;
    }

    const FString BattleDefinitionValue =
        UGameplayStatics::ParseOption(Options, TEXT("BattleDefinition"));
    if (!BattleDefinitionValue.IsEmpty())
    {
        const FPrimaryAssetId BattleDefinitionId(
            ULastFPSBattleDefinition::PrimaryAssetType,
            FName(*BattleDefinitionValue));
        if (!BattleDefinitionId.IsValid())
        {
            ErrorMessage = FString::Printf(
                TEXT("전투 정의 식별자 이름이 올바르지 않습니다: %s"),
                *BattleDefinitionValue);
        }
        else
        {
            const FSoftObjectPath DefinitionPath =
                UAssetManager::Get().GetPrimaryAssetPath(BattleDefinitionId);
            ActiveBattleDefinition = Cast<ULastFPSBattleDefinition>(
                DefinitionPath.TryLoad());
            if (!ActiveBattleDefinition)
            {
                ErrorMessage = FString::Printf(
                    TEXT("전투 정의를 불러오지 못했습니다: %s"),
                    *BattleDefinitionValue);
            }
            else if (ULastFPSDestinationContentSet* DefinitionContentSet =
                ActiveBattleDefinition->ContentSet.LoadSynchronous())
            {
                DestinationContentSet = DefinitionContentSet;
            }
            else
            {
                ErrorMessage = FString::Printf(
                    TEXT("전투 정의의 ContentSet을 불러오지 못했습니다: %s"),
                    *BattleDefinitionValue);
            }
        }
    }

    if (!ErrorMessage.IsEmpty())
    {
        UE_LOG(LogLastFPSGameMode, Error, TEXT("InitGame 실패: %s"), *ErrorMessage);
    }

}

void ALastFPSGameModeBase::InitGameState()
{
    Super::InitGameState();

    if (ALastFPSGameStateBase* LastFPSGameState =
        GetGameState<ALastFPSGameStateBase>())
    {
        const FGameplayTagContainer ContextTags = DestinationContentSet
            ? DestinationContentSet->ContextTags
            : FGameplayTagContainer();
        LastFPSGameState->SetDestinationContextTags(ContextTags);
    }

    // 실패 구독은 콘텐츠 로딩과 무관한 책임이다 — 아래 조기 반환에 가려지지 않도록 먼저 건다.
    BindEncounterFailureEvents();

    // GameState 스폰 직후이자 BeginPlay 이전이라 첫 PostLogin 보다 확실히 앞선다.
    DestinationContentComponent = GameState
        ? GameState->FindComponentByClass<ULastFPSDestinationContentComponent>()
        : nullptr;

    ULastFPSDestinationContentComponent* Content = DestinationContentComponent.Get();
    if (!Content)
    {
        UE_LOG(LogLastFPSGameMode, Log,
            TEXT("게이트 컴포넌트가 없어 콘텐츠 대기 없이 진행합니다: %s"), *GetNameSafe(GameState));
        return;
    }

    // InitGameState 는 Reset() 에서도 호출되므로 중복 구독과 중복 스폰을 막는다.
    if (AssetsLoadedHandle.IsValid() || ContentReadyHandle.IsValid())
    {
        return;
    }

    AssetsLoadedHandle = Content->OnAssetsLoaded.AddUObject(
        this, &ALastFPSGameModeBase::HandleDestinationAssetsLoaded);
    ContentReadyHandle = Content->OnContentReady.AddUObject(
        this, &ALastFPSGameModeBase::HandleDestinationContentReady);
    Content->StartContentLoad(DestinationContentSet);
}

void ALastFPSGameModeBase::BindEncounterFailureEvents()
{
    UWorld* World = GetWorld();
    ULastFPSRoomEncounterSubsystem* EncounterSubsystem = World
        ? World->GetSubsystem<ULastFPSRoomEncounterSubsystem>()
        : nullptr;
    if (!EncounterSubsystem)
    {
        // 인카운터가 없는 맵(허브 등)에서는 구독할 대상이 없다 — 정상 경로다.
        return;
    }

    EncounterSubsystem->OnEncounterFailed.AddUniqueDynamic(
        this, &ALastFPSGameModeBase::HandleEncounterFailed);
}

void ALastFPSGameModeBase::UnbindEncounterFailureEvents()
{
    UWorld* World = GetWorld();
    if (ULastFPSRoomEncounterSubsystem* EncounterSubsystem = World
        ? World->GetSubsystem<ULastFPSRoomEncounterSubsystem>()
        : nullptr)
    {
        EncounterSubsystem->OnEncounterFailed.RemoveDynamic(
            this, &ALastFPSGameModeBase::HandleEncounterFailed);
    }
}

void ALastFPSGameModeBase::HandleEncounterFailed(const FName EncounterId)
{
    HandleMissionFailed(EncounterId);
}

void ALastFPSGameModeBase::HandleMissionFailed(const FName FailedEncounterId)
{
    if (!HasAuthority() || bMissionFailedHandled)
    {
        return;
    }
    bMissionFailedHandled = true;

    UE_LOG(LogLastFPSGameMode, Log,
        TEXT("미션 실패: Encounter=%s, 귀환=%s"),
        *FailedEncounterId.ToString(),
        bReturnToHubOnMissionFailed ? TEXT("예") : TEXT("아니오"));

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 실패 화면은 로컬 UI라 각 컨트롤러가 자기 화면을 연다.
    if (MissionFailedScreenTag.IsValid())
    {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            if (ALastFPSPlayerController* Controller = Cast<ALastFPSPlayerController>(It->Get()))
            {
                Controller->OpenScreen(MissionFailedScreenTag);
            }
        }
    }

    if (!bReturnToHubOnMissionFailed)
    {
        return;
    }

    if (MissionFailedReturnDelay > 0.f)
    {
        World->GetTimerManager().SetTimer(
            MissionFailedReturnTimerHandle,
            this,
            &ALastFPSGameModeBase::ExecuteMissionFailedReturn,
            MissionFailedReturnDelay,
            false);
        return;
    }

    ExecuteMissionFailedReturn();
}

void ALastFPSGameModeBase::ExecuteMissionFailedReturn()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 허브는 세션 밖 로컬 월드이므로 서버 이동이 아니라 각 소유 클라이언트가 독립적으로 연다.
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (ALastFPSPlayerController* Controller =
            Cast<ALastFPSPlayerController>(It->Get()))
        {
            Controller->ClientReturnToHub();
        }
    }
}

void ALastFPSGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ULastFPSDestinationContentComponent* Content = DestinationContentComponent.Get())
    {
        Content->OnAssetsLoaded.Remove(AssetsLoadedHandle);
        Content->OnContentReady.Remove(ContentReadyHandle);
    }
    AssetsLoadedHandle.Reset();
    ContentReadyHandle.Reset();

    UnbindEncounterFailureEvents();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MissionFailedReturnTimerHandle);
    }

    // Seamless Travel에서는 PlayerController가 다음 월드까지 살아남을 수 있어 입력 차단을 반드시 상쇄한다.
    for (const TWeakObjectPtr<APlayerController>& ControllerPtr : WarmupInputBlockedControllers)
    {
        SetLocalWarmupInputBlocked(ControllerPtr.Get(), false);
    }
    WarmupInputBlockedControllers.Reset();

    Super::EndPlay(EndPlayReason);
}

bool ALastFPSGameModeBase::IsDestinationContentReady() const
{
    const ULastFPSDestinationContentComponent* Content = DestinationContentComponent.Get();
    return !Content || Content->IsContentReady();
}

void ALastFPSGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    if (!IsDestinationContentReady())
    {
        // 스폰을 버리는 것이 아니라 미룬다. 에셋 준비 콜백이 실제 Pawn을 생성한다.
        return;
    }

    Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void ALastFPSGameModeBase::HandleDestinationAssetsLoaded()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    TArray<AActor*> WarmupActors;
    FTransform PoolWarmupTransform = FTransform::Identity;
    bool bHasPoolWarmupTransform = false;
    int32 RestartedPlayers = 0;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && PC->GetPawn() == nullptr && PlayerCanRestart(PC))
        {
            RestartPlayer(PC);
            ++RestartedPlayers;
        }

        if (PC && PC->GetPawn())
        {
            WarmupActors.Add(PC->GetPawn());
            if (!bHasPoolWarmupTransform)
            {
                PoolWarmupTransform = PC->GetPawn()->GetActorTransform();
                bHasPoolWarmupTransform = true;
            }
            SetLocalWarmupInputBlocked(PC, true);
        }
    }

    TArray<AActor*> PoolWarmupActors;
    if (ULastFPSDestinationContentComponent* Content =
        DestinationContentComponent.Get())
    {
        Content->BeginActorPoolPreparation();
    }
    if (ULastFPSActorPoolSubsystem* ActorPool =
        World->GetSubsystem<ULastFPSActorPoolSubsystem>())
    {
        const ULastFPSActorPoolProfile* PoolProfile = DestinationContentSet
            ? DestinationContentSet->FindFeature<ULastFPSActorPoolProfile>()
            : nullptr;
        ActorPool->PreparePools(
            PoolProfile,
            PoolWarmupTransform,
            PoolWarmupActors);
        WarmupActors.Append(PoolWarmupActors);
    }
    if (ULastFPSDestinationContentComponent* Content =
        DestinationContentComponent.Get())
    {
        Content->CompleteActorPoolPreparation();
    }

    UE_LOG(LogLastFPSGameMode, Log,
        TEXT("에셋 준비 완료 — 플레이어 %d명 스폰, 렌더 준비 Actor %d개(풀 %d개)"),
        RestartedPlayers,
        WarmupActors.Num(),
        PoolWarmupActors.Num());

    if (ULastFPSDestinationContentComponent* Content = DestinationContentComponent.Get())
    {
        Content->BeginRenderWarmup(
            WarmupActors,
            FSimpleDelegate::CreateUObject(
                this,
                &ThisClass::HandleRenderWarmupCompleted));
    }
}

void ALastFPSGameModeBase::HandleRenderWarmupCompleted()
{
    if (UWorld* World = GetWorld())
    {
        if (ULastFPSActorPoolSubsystem* ActorPool =
            World->GetSubsystem<ULastFPSActorPoolSubsystem>())
        {
            ActorPool->FinalizePrewarm();
        }
    }
}

void ALastFPSGameModeBase::HandleDestinationContentReady()
{
    for (const TWeakObjectPtr<APlayerController>& ControllerPtr : WarmupInputBlockedControllers)
    {
        SetLocalWarmupInputBlocked(ControllerPtr.Get(), false);
    }
    WarmupInputBlockedControllers.Reset();

    // 렌더 준비 도중 접속한 플레이어가 있다면 최종 준비 시점에 빠짐없이 생성한다.
    int32 LateRestartedPlayers = 0;
    if (UWorld* World = GetWorld())
    {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (PC && PC->GetPawn() == nullptr && PlayerCanRestart(PC))
            {
                RestartPlayer(PC);
                ++LateRestartedPlayers;
            }
        }
    }

    UE_LOG(LogLastFPSGameMode, Log,
        TEXT("콘텐츠 및 플레이어 렌더 준비 완료 — 후발 플레이어 %d명 스폰"),
        LateRestartedPlayers);
}

void ALastFPSGameModeBase::SetLocalWarmupInputBlocked(
    APlayerController* PlayerController,
    const bool bBlocked)
{
    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return;
    }

    PlayerController->SetIgnoreMoveInput(bBlocked);
    PlayerController->SetIgnoreLookInput(bBlocked);

    if (bBlocked)
    {
        WarmupInputBlockedControllers.AddUnique(PlayerController);
    }
}

void ALastFPSGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
}

void ALastFPSGameModeBase::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
}

UClass* ALastFPSGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    if (ALastFPSPlayerState* LastPS = InController ? InController->GetPlayerState<ALastFPSPlayerState>() : nullptr)
    {
        int32 SelectedIndex = LastPS->GetSelectedCharacterIndex();
        if (ULastFPSGameInstance* LastGI = GetGameInstance<ULastFPSGameInstance>())
        {
            int32 RestoredIndex = 0;
            if (LastGI->TryGetSelectedCharacterIndex(LastPS->GetPlayerName(), RestoredIndex))
            {
                SelectedIndex = RestoredIndex;
            }
        }

        LastPS->Auth_SetSelectedCharacterIndex(SelectedIndex);

        if (const ULastFPSCharacterDefinition* Definition = GetCharacterDefinitionForIndex(SelectedIndex))
        {
            if (Definition->PawnClass)
            {
                return Definition->PawnClass;
            }
        }
    }

    if (const ALastFPSPlayerController* LastPC = Cast<ALastFPSPlayerController>(InController))
    {
        if (TSubclassOf<APawn> SelectedClass = LastPC->GetSelectedCharacterClass())
        {
            return SelectedClass;
        }
    }

    return Super::GetDefaultPawnClassForController_Implementation(InController);
}

const ULastFPSCharacterDefinition* ALastFPSGameModeBase::GetCharacterDefinitionForIndex(const int32 CharacterIndex) const
{
    if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
    {
        if (const ULastFPSCharacterRoster* Roster = GI->GetCharacterRoster())
        {
            return Roster->GetDefinition(CharacterIndex);
        }
    }
    return nullptr;
}

bool ALastFPSGameModeBase::ApplyCharacterDefinitionToAbilitySystem(
    UAbilitySystemComponent* ASC,
    const ULastFPSCharacterDefinition* CharacterDefinition) const
{
    if (!ASC || !CharacterDefinition)
    {
        return false;
    }

    // Hero/Enemy 서브클래스의 virtual 이 각자 전용 스타트업 처리를 덧붙인다.
    CharacterDefinition->GiveToAbilitySystem(ASC);

    return CharacterDefinition->StatData != nullptr;
}

void ALastFPSGameModeBase::ApplyLevelRestrictionsToAbilitySystem(UAbilitySystemComponent* ASC) const
{
    if (!ASC || !LevelRestrictionEffect)
    {
        return;
    }

    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    Context.AddSourceObject(this);

    const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(LevelRestrictionEffect, 1.f, Context);
    if (Spec.IsValid())
    {
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}

int32 ALastFPSGameModeBase::GetTotalConnectedPlayers() const
{
    return GameState ? GameState->PlayerArray.Num() : 0;
}

void ALastFPSGameModeBase::DebugFlow(const FString& Message, FColor Color) const
{
    UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, Color, Message);
    }
}
