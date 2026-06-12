#pragma once

#include "CoreMinimal.h"
#include "LastFPSCharacterTypes.generated.h"

UENUM(BlueprintType)
enum class ELastFPSCharacterType : uint8
{
	Player,
	Dummy,
	Enemy,
	NPC
};

UENUM(BlueprintType)
enum class ELastFPSAIBehaviorType : uint8
{
	None,
	StaticTarget,
	MovingTarget,
	Patrol,
	AttackTarget
};
