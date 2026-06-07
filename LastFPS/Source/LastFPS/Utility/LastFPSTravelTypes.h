#pragma once

#include "CoreMinimal.h"
#include "LastFPSTravelTypes.generated.h"

/** 아웃게임 맵 이동 대상 */
UENUM(BlueprintType)
enum class ELastFPSTravelDestination : uint8
{
	MainMenu,
	CharacterSelect,
	Hub,
};
