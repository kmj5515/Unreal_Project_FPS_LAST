#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSBossKnockdownData.generated.h"

class UAnimMontage;

/** 약점 파괴 넉다운의 몽타주 구간과 유지 시간을 정의하는 불변 설정이다. */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSBossKnockdownData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Knockdown")
	TObjectPtr<UAnimMontage> KnockdownMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Knockdown", meta=(ClampMin="0.01"))
	float MontagePlayRate = 1.f;

	/** Start 구간 이후 Loop 구간에 머무르는 최소 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Knockdown", meta=(ClampMin="0.0", Units="s"))
	float LoopDuration = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Knockdown|Sections")
	FName StartSectionName = TEXT("Start");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Knockdown|Sections")
	FName LoopSectionName = TEXT("Loop");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Knockdown|Sections")
	FName EndSectionName = TEXT("End");
};
