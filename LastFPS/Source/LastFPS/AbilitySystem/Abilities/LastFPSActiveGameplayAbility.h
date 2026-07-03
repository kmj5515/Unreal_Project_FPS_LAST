#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "LastFPSActiveGameplayAbility.generated.h"

UCLASS(Abstract)
class LASTFPS_API ULastFPSActiveGameplayAbility : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	ULastFPSActiveGameplayAbility();
};
