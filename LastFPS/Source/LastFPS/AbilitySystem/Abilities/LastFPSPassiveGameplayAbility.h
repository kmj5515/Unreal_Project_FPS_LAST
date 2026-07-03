#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "LastFPSPassiveGameplayAbility.generated.h"

UCLASS(Abstract)
class LASTFPS_API ULastFPSPassiveGameplayAbility : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	ULastFPSPassiveGameplayAbility();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

private:
	void TryActivatePassiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;
};
