#include "Game/LastFPSMatchGameState.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

ALastFPSMatchGameState::ALastFPSMatchGameState()
{
}

void ALastFPSMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSMatchGameState, MatchTimeRemaining);
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

void ALastFPSMatchGameState::Auth_SetMatchResult(APlayerState* InWinner, const FString& InReason)
{
    if (!HasAuthority())
    {
        return;
    }

    WinnerPlayerState = InWinner;
    EndReason         = InReason;
    bMatchEnded       = true;

    // 서버 자신에서도 동일하게 결과 화면 흐름이 시작되도록 OnRep을 직접 호출
    OnRep_MatchEnded();
}

void ALastFPSMatchGameState::OnRep_MatchEnded()
{
    if (bMatchEnded)
    {
        OnMatchEnded.Broadcast();
    }
}
