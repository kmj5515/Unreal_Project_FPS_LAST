#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "LastFPSWeaponBalanceData.generated.h"

/** 무기 정의의 에셋 구성과 분리해 관리하는 무기 밸런스 행이다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSWeaponBalanceData : public FTableRowBase
{
	GENERATED_BODY()

	/** ±20% 피해 범위를 계산하기 전의 기준 피해량이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance", meta=(ClampMin="0.0"))
	float Damage = 0.f;

	/** 연속 발사 사이의 최소 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance", meta=(ClampMin="0.01", Units="s"))
	float FireInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance", meta=(ClampMin="0.0", Units="cm"))
	float AimTraceRange = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance")
	ELastFPSDamageElement DamageElement = ELastFPSDamageElement::Physical;

	/** 무기 고유 수치는 Gameplay Tag를 키로 추가해 공통 행 구조 변경을 줄인다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance")
	TMap<FGameplayTag, float> Parameters;

	float GetParameter(const FGameplayTag& ParameterTag, const float FallbackValue) const
	{
		const float* Value = Parameters.Find(ParameterTag);
		return Value ? *Value : FallbackValue;
	}
};
