#pragma once

#include "CoreMinimal.h"
#include "LastFPSTeamTypes.generated.h"

/** 게임에서 사용하는 기본 진영 식별자다. */
UENUM()
enum class ELastFPSTeam : uint8
{
	Enemy = 1
};
