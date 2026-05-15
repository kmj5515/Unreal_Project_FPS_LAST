#include "Game/LastFPSMatchGameState.h"

#include "Game/LastFPSPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

ALastFPSMatchGameState::ALastFPSMatchGameState()
{
}

void ALastFPSMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSMatchGameState, MatchTimeRemaining);
    DOREPLIFETIME(ALastFPSMatchGameState, bDropIntroActive);
    DOREPLIFETIME(ALastFPSMatchGameState, bMatchEnded);
    DOREPLIFETIME(ALastFPSMatchGameState, WinnerPlayerState);
    DOREPLIFETIME(ALastFPSMatchGameState, EndReason);
}

void ALastFPSMatchGameState::Auth_SetMatchTimeRemaining(float NewSeconds)
{
    if (!HasAuthority())
    {
        return;
    }

    MatchTimeRemaining = FMath::Max(0.f, NewSeconds);
}

void ALastFPSMatchGameState::Auth_SetDropIntroActive(bool bActive)
{
    if (!HasAuthority())
    {
        return;
    }

    bDropIntroActive = bActive;
}

void ALastFPSMatchGameState::Auth_SetMatchResult(APlayerState* InWinner, const FString& InReason)
{
    if (!HasAuthority())
    {
        return;
    }

    WinnerPlayerState = InWinner;
    EndReason         = InReason;
    bMatchEnded       = true;

    // Listen server: RepNotify와 동일하게 결과 UI 흐름 시작
    OnRep_MatchEnded();
}

void ALastFPSMatchGameState::OnRep_MatchEnded()
{
    if (bMatchEnded)
    {
        OnMatchEnded.Broadcast();
    }
}

void ALastFPSMatchGameState::Auth_BroadcastKillFeed(
    ALastFPSPlayerState* KillerPS,
    ALastFPSPlayerState* VictimPS)
{
    if (!HasAuthority() || !KillerPS || !VictimPS)
        return;

    Multicast_KillFeed(KillerPS->GetPlayerName(), VictimPS->GetPlayerName());
}

void ALastFPSMatchGameState::Multicast_KillFeed_Implementation(
    const FString& KillerName,
    const FString& VictimName)
{
    OnKillFeed.Broadcast(KillerName, VictimName);
}
