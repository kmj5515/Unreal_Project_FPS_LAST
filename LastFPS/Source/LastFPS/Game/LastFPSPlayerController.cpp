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

void ALastFPSPlayerController::BeginPlay()
{
    Super::BeginPlay();
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}

int32 ALastFPSPlayerController::ClampSelectedCharacterIndex(int32 NewIndex) const
{
    const int32 MaxIndex = SelectableCharacterClasses.Num() > 0 ? (SelectableCharacterClasses.Num() - 1) : NewIndex;
    return FMath::Clamp(NewIndex, 0, MaxIndex);
}

void ALastFPSPlayerController::SyncSelectedCharacterState(int32 CharacterIndex)
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
    const int32 ClampedIndex = ClampSelectedCharacterIndex(NewIndex);
    SelectedCharacterIndex = ClampedIndex;
    ServerSetSelectedCharacterIndex(ClampedIndex);
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
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
    const int32 ClampedIndex = ClampSelectedCharacterIndex(NewIndex);

    SelectedCharacterIndex = ClampedIndex;
    SyncSelectedCharacterState(ClampedIndex);
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}

void ALastFPSPlayerController::OnRep_SelectedCharacterIndex()
{
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}
