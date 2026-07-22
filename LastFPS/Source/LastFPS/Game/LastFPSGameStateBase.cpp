#include "Game/LastFPSGameStateBase.h"

#include "Game/Loading/LastFPSDestinationContentComponent.h"

ALastFPSGameStateBase::ALastFPSGameStateBase()
{
    DestinationContent = CreateDefaultSubobject<ULastFPSDestinationContentComponent>(TEXT("DestinationContent"));
}
