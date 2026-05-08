#include "Game/LastFPSMatchGameState.h"

#include "Net/UnrealNetwork.h"

namespace
{
    constexpr int32 LastFPSValidTeamCount = 4; // TeamA~TeamD
}

ALastFPSMatchGameState::ALastFPSMatchGameState()
{
    TeamScores.Init(0, LastFPSValidTeamCount);
}

void ALastFPSMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSMatchGameState, TeamScores);
}

int32 ALastFPSMatchGameState::GetTeamScore(ELastFPSTeam Team) const
{
    const int32 Index = static_cast<int32>(Team);
    return TeamScores.IsValidIndex(Index) ? TeamScores[Index] : 0;
}

void ALastFPSMatchGameState::Auth_AddTeamScore(ELastFPSTeam Team, int32 Delta)
{
    if (!HasAuthority())
    {
        return;
    }

    const int32 Index = static_cast<int32>(Team);
    if (!TeamScores.IsValidIndex(Index))
    {
        return;
    }

    TeamScores[Index] = FMath::Max(0, TeamScores[Index] + Delta);
}
