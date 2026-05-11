#include "Game/LastFPSMatchGameState.h"

#include "Net/UnrealNetwork.h"

ALastFPSMatchGameState::ALastFPSMatchGameState()
{
}

void ALastFPSMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSMatchGameState, MatchTimeRemaining);
}

void ALastFPSMatchGameState::Auth_SetMatchTimeRemaining(float NewSeconds)
{
    if (!HasAuthority())
    {
        return;
    }

    MatchTimeRemaining = FMath::Max(0.f, NewSeconds);
}
