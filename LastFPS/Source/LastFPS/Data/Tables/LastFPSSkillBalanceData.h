#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LastFPSSkillBalanceData.generated.h"

/** 스킬의 변경 가능한 밸런스 수치만 저장하는 DataTable 행이다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSSkillBalanceData : public FTableRowBase
{
	GENERATED_BODY()

	/** 최종 범위가 아니라 ±20% 공식을 적용하기 전의 기준 데미지다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance", meta=(ClampMin="0.0"))
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance", meta=(ClampMin="0.0", Units="s"))
	float Cooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance", meta=(ClampMin="0.0"))
	float Cost = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance", meta=(ClampMin="0.0", Units="cm"))
	float Range = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance", meta=(ClampMin="0.0", Units="cm"))
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance", meta=(ClampMin="0.0", Units="s"))
	float Duration = 0.f;

	/** 스킬 고유 수치는 Gameplay Tag를 키로 추가해 공통 행 구조 변경을 줄인다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance")
	TMap<FGameplayTag, float> Parameters;

	float GetParameter(const FGameplayTag& ParameterTag, const float FallbackValue) const
	{
		const float* Value = Parameters.Find(ParameterTag);
		return Value ? *Value : FallbackValue;
	}
};
