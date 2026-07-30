#include "Game/LastFPSGameStateBase.h"

#include "Game/Loading/LastFPSDestinationContentComponent.h"

#include "Net/UnrealNetwork.h"

ALastFPSGameStateBase::ALastFPSGameStateBase()
{
    DestinationContent =
        CreateDefaultSubobject<ULastFPSDestinationContentComponent>(
            TEXT("DestinationContent"));
}

void ALastFPSGameStateBase::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSGameStateBase, DestinationContextTags);
}

void ALastFPSGameStateBase::SetDestinationContextTags(
    const FGameplayTagContainer& NewContextTags)
{
    if (!HasAuthority() || DestinationContextTags == NewContextTags)
    {
        return;
    }

    DestinationContextTags = NewContextTags;
    OnDestinationContextChanged.Broadcast(DestinationContextTags);
    ForceNetUpdate();
}

void ALastFPSGameStateBase::OnRep_DestinationContextTags()
{
    OnDestinationContextChanged.Broadcast(DestinationContextTags);
}
