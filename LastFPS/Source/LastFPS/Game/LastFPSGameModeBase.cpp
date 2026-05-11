#include "Game/LastFPSGameModeBase.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
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
    if (const ALastFPSPlayerState* LastPS = InController ? InController->GetPlayerState<ALastFPSPlayerState>() : nullptr)
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

        if (CharacterPawnClasses.IsValidIndex(SelectedIndex) && CharacterPawnClasses[SelectedIndex])
        {
            return CharacterPawnClasses[SelectedIndex];
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
