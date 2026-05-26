#pragma once

#include "GameUIManagerSubsystem.h"
#include "LastFPSUIManagerSubsystem.generated.h"

/** LastFPS UI manager — creates PrimaryGameLayout per local player via UGameUIPolicy */
UCLASS()
class LASTFPS_API ULastFPSUIManagerSubsystem : public UGameUIManagerSubsystem
{
	GENERATED_BODY()
};
