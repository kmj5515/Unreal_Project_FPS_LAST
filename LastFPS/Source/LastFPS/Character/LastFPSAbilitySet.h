#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSAbilitySet.generated.h"

class UGameplayAbility;
class UGameplayEffect;

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS")
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;
};
