#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSCharacterAcceleratorData.generated.h"

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSCharacterAcceleratorData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Accelerator")
	FName AcceleratorId;
};
