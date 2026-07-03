#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "LastFPSConfirmableAbility.generated.h"

UINTERFACE(MinimalAPI)
class ULastFPSConfirmableAbility : public UInterface
{
	GENERATED_BODY()
};

class LASTFPS_API ILastFPSConfirmableAbility
{
	GENERATED_BODY()

public:
	virtual bool CanConfirmAbilityInput(FGameplayTag InputTag) const = 0;
	virtual bool ConfirmAbilityInput(FGameplayTag InputTag) = 0;
	virtual bool CanCancelAbilityInput(FGameplayTag InputTag) const { return false; }
	virtual bool CancelAbilityInput(FGameplayTag InputTag) { return false; }
	virtual bool ShouldBlockAbilityInputRelease(FGameplayTag InputTag) const { return false; }
};
