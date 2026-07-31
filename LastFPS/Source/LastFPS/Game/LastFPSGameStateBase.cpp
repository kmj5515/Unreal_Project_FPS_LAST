#include "Game/LastFPSGameStateBase.h"

#include "Data/Definitions/LastFPSDestinationContentSet.h"
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
    DOREPLIFETIME(ALastFPSGameStateBase, DestinationContentSetId);
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

void ALastFPSGameStateBase::SetDestinationContentSet(
    const ULastFPSDestinationContentSet* NewContentSet)
{
    if (!HasAuthority())
    {
        return;
    }

    const FPrimaryAssetId NewContentSetId = NewContentSet
        ? NewContentSet->GetPrimaryAssetId()
        : FPrimaryAssetId();
    if (DestinationContentSetId == NewContentSetId)
    {
        return;
    }

    DestinationContentSetId = NewContentSetId;
    ForceNetUpdate();
}

void ALastFPSGameStateBase::OnRep_DestinationContextTags()
{
    OnDestinationContextChanged.Broadcast(DestinationContextTags);
}

void ALastFPSGameStateBase::OnRep_DestinationContentSetId()
{
    if (DestinationContent && DestinationContentSetId.IsValid())
    {
        DestinationContent->StartContentLoad(DestinationContentSetId);
    }
}
