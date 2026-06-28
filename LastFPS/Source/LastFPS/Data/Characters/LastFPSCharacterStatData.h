#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSCharacterStatData.generated.h"

class UAbilitySystemComponent;

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSCharacterStatData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin=0))
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin=1))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin=0))
	float Stamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin=1))
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin=0))
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin=0))
	float Defense = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin=0))
	float MoveSpeed = 500.f;

	bool ApplyToAbilitySystem(UAbilitySystemComponent* ASC) const;
};
