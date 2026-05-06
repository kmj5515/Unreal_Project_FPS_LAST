#include "Game/LastFPSMatchGameMode.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void ALastFPSMatchGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("[Match] MatchGameMode BeginPlay"));
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, TEXT("[Match] MatchGameMode BeginPlay"));
    }
}

void ALastFPSMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    const FString Message = FString::Printf(
        TEXT("[Match] Player Joined: %s (Total: %d)"),
        *NewPlayer->GetName(),
        GetTotalConnectedPlayers());

    UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan, Message);
    }
}
