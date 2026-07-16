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

	/** 공격 판정과 AI 이동 수용 반경에 사용하는 기본 사거리(cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Combat", meta=(ClampMin=0, Units="cm"))
	float AttackRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Critical", meta=(ClampMin=0, ClampMax=100, Units="%"))
	float CriticalChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Critical", meta=(ClampMin=100, Units="%"))
	float CriticalDamagePercent = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin=0))
	float Defense = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Damage", meta=(ClampMin=0))
	float PhysicalDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Damage", meta=(ClampMin=0))
	float FireDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Damage", meta=(ClampMin=0))
	float IceDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Damage", meta=(ClampMin=0))
	float ElectricDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Damage", meta=(ClampMin=0))
	float PoisonDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin=0))
	float MoveSpeed = 500.f;

	bool ApplyToAbilitySystem(UAbilitySystemComponent* ASC) const;
};
