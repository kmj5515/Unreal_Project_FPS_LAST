#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Character/LastFPSCharacterTypes.h"
#include "LastFPSAIProfile.generated.h"

class AAIController;

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSAIProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	ELastFPSAIBehaviorType BehaviorType = ELastFPSAIBehaviorType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	TSubclassOf<AAIController> AIControllerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin=0))
	float DetectionRange = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin=0))
	float AttackRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin=0))
	float ReactionDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	bool bCanAttack = false;
};
