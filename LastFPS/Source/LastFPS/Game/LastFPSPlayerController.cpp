#include "Game/LastFPSPlayerController.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSLobbyGameMode.h"
#include "Game/LastFPSPlayerState.h"
#include "Net/UnrealNetwork.h"

void ALastFPSPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSPlayerController, SelectedCharacterIndex);
}

void ALastFPSPlayerController::SetLobbyReady(bool bReady)
{
    bLobbyReady = bReady;
    ServerSetLobbyReady(bReady);
}

void ALastFPSPlayerController::ServerSetLobbyReady_Implementation(bool bReady)
{
    bLobbyReady = bReady;

    if (UWorld* World = GetWorld())
    {
        if (ALastFPSLobbyGameMode* LobbyGameMode = Cast<ALastFPSLobbyGameMode>(World->GetAuthGameMode()))
        {
            LobbyGameMode->SetPlayerReady(this, bReady);
        }
    }
}

void ALastFPSPlayerController::SetSelectedCharacterIndex(int32 NewIndex)
{
    const int32 MaxIndex = SelectableCharacterClasses.Num() > 0 ? (SelectableCharacterClasses.Num() - 1) : NewIndex;
    const int32 ClampedIndex = FMath::Clamp(NewIndex, 0, MaxIndex);
    SelectedCharacterIndex = ClampedIndex;
    ServerSetSelectedCharacterIndex(ClampedIndex);
}

TSubclassOf<APawn> ALastFPSPlayerController::GetSelectedCharacterClass() const
{
    if (!SelectableCharacterClasses.IsValidIndex(SelectedCharacterIndex))
    {
        return nullptr;
    }

    return SelectableCharacterClasses[SelectedCharacterIndex];
}

void ALastFPSPlayerController::ServerSetSelectedCharacterIndex_Implementation(int32 NewIndex)
{
    const int32 MaxIndex = SelectableCharacterClasses.Num() > 0 ? (SelectableCharacterClasses.Num() - 1) : NewIndex;
    const int32 ClampedIndex = FMath::Clamp(NewIndex, 0, MaxIndex);

    if (SelectedCharacterIndex == ClampedIndex)
    {
        if (ALastFPSPlayerState* LastPS = GetPlayerState<ALastFPSPlayerState>())
        {
            LastPS->Auth_SetSelectedCharacterIndex(ClampedIndex);

            if (ULastFPSGameInstance* LastGI = GetGameInstance<ULastFPSGameInstance>())
            {
                const FString PlayerKey = LastPS->GetPlayerName();
                LastGI->SaveSelectedCharacterIndex(PlayerKey, ClampedIndex);
            }
        }
        return;
    }

    SelectedCharacterIndex = ClampedIndex;

    if (ALastFPSPlayerState* LastPS = GetPlayerState<ALastFPSPlayerState>())
    {
        LastPS->Auth_SetSelectedCharacterIndex(ClampedIndex);

        if (ULastFPSGameInstance* LastGI = GetGameInstance<ULastFPSGameInstance>())
        {
            const FString PlayerKey = LastPS->GetPlayerName();
            LastGI->SaveSelectedCharacterIndex(PlayerKey, ClampedIndex);
        }
    }
}

void ALastFPSPlayerController::OnRep_SelectedCharacterIndex()
{
}
