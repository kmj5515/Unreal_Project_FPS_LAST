#include "Game/LastFPSGameModeBase.h"
#include "Character/LastFPSCharacterStatData.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Game/LastFPSCharacterDefinition.h"
#include "Game/LastFPSCharacterRoster.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

ALastFPSGameModeBase::ALastFPSGameModeBase()
{
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

    if (CharacterDefinition->StatData && CharacterDefinition->StatData->ApplyToAbilitySystem(ASC))
    {
        return true;
    }

    return false;
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
