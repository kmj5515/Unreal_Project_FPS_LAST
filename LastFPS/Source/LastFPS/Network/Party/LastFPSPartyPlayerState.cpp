#include "LastFPSPartyPlayerState.h"
#include "Net/UnrealNetwork.h"

ALastFPSPartyPlayerState::ALastFPSPartyPlayerState()
{
	bIsReady = false;
	bIsHost = false;
}

void ALastFPSPartyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALastFPSPartyPlayerState, bIsReady);
	DOREPLIFETIME(ALastFPSPartyPlayerState, bIsHost);
}

void ALastFPSPartyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ALastFPSPartyPlayerState* PartyPS = Cast<ALastFPSPartyPlayerState>(PlayerState);
	if (PartyPS)
	{
		PartyPS->bIsHost = bIsHost;
		PartyPS->bIsReady = bIsReady;
	}
}

void ALastFPSPartyPlayerState::SetPartyReady(bool bInReady)
{
	if (HasAuthority())
	{
		bIsReady = bInReady;
		OnRep_IsReady();
	}
}

void ALastFPSPartyPlayerState::SetPartyHost(bool bInHost)
{
	if (HasAuthority())
	{
		bIsHost = bInHost;
		OnRep_IsHost();
	}
}

void ALastFPSPartyPlayerState::OnRep_IsReady()
{
	OnPartyStateChanged.Broadcast(this);
}

void ALastFPSPartyPlayerState::OnRep_IsHost()
{
	OnPartyStateChanged.Broadcast(this);
}
