#include "Game/LastFPSGameModeBase.h"
#include "AbilitySystemComponent.h"
#include "Data/Characters/LastFPSCharacterStatData.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Definitions/LastFPSCharacterRoster.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSGameStateBase.h"
#include "Game/LastFPSPlayerController.h"
#include "Game/LastFPSPlayerState.h"
#include "Game/Loading/LastFPSDestinationContentComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSGameMode, Log, All);

ALastFPSGameModeBase::ALastFPSGameModeBase()
{
    // 플레이어 ASC(PlayerState 소유)·스탯·캐릭터 선택이 ALastFPSPlayerState 를 전제하므로 명시적으로 지정한다.
    // (미지정 시 AGameModeBase 기본 APlayerState 가 쓰여 GetPlayerState<ALastFPSPlayerState> 가 항상 null → ASC/스탯 소실.)
    PlayerStateClass = ALastFPSPlayerState::StaticClass();

    // 로딩 게이트 컴포넌트의 부착 지점. 같은 이유로 명시 지정한다.
    GameStateClass = ALastFPSGameStateBase::StaticClass();
}

void ALastFPSGameModeBase::InitGameState()
{
    Super::InitGameState();

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

void ALastFPSGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ULastFPSDestinationContentComponent* Content = DestinationContentComponent.Get())
    {
        Content->OnAssetsLoaded.Remove(AssetsLoadedHandle);
        Content->OnContentReady.Remove(ContentReadyHandle);
    }
    AssetsLoadedHandle.Reset();
    ContentReadyHandle.Reset();

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
            SetLocalWarmupInputBlocked(PC, true);
        }
    }

    UE_LOG(LogLastFPSGameMode, Log,
        TEXT("에셋 준비 완료 — 대기 중이던 플레이어 %d명 스폰, 렌더 준비 Actor %d개"),
        RestartedPlayers,
        WarmupActors.Num());

    if (ULastFPSDestinationContentComponent* Content = DestinationContentComponent.Get())
    {
        Content->BeginRenderWarmup(WarmupActors);
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
