#include "Game/LastFPSGameModeBase.h"
#include "AbilitySystemComponent.h"
#include "Data/Characters/LastFPSCharacterStatData.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Definitions/LastFPSCharacterRoster.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

ALastFPSGameModeBase::ALastFPSGameModeBase()
{
    // 플레이어 ASC(PlayerState 소유)·스탯·캐릭터 선택이 ALastFPSPlayerState 를 전제하므로 명시적으로 지정한다.
    // (미지정 시 AGameModeBase 기본 APlayerState 가 쓰여 GetPlayerState<ALastFPSPlayerState> 가 항상 null → ASC/스탯 소실.)
    PlayerStateClass = ALastFPSPlayerState::StaticClass();
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
