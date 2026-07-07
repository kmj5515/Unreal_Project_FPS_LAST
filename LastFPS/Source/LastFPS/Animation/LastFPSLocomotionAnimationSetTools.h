#pragma once

#include "CoreMinimal.h"
#include "Animation/LastFPSLocomotionAnimationSet.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LastFPSLocomotionAnimationSetTools.generated.h"

UCLASS()
class LASTFPS_API ULastFPSLocomotionAnimationSetTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LastFPS|Animation Tools", meta=(DevelopmentOnly, DisplayName="Auto Fill Locomotion Animation Set"))
	static int32 AutoFillLocomotionAnimationSet(
		ULastFPSLocomotionAnimationSet* AnimationSet,
		const FString& ContentPath,
		bool bOverwriteExisting,
		bool bClearBeforeFill);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Animation Tools", meta=(DevelopmentOnly, DisplayName="Auto Fill Locomotion Animation Set With Name Filter"))
	static int32 AutoFillLocomotionAnimationSetWithNameFilter(
		ULastFPSLocomotionAnimationSet* AnimationSet,
		const FString& ContentPath,
		const FString& RequiredNameText,
		bool bOverwriteExisting,
		bool bClearBeforeFill);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Animation Tools", meta=(DevelopmentOnly, DisplayName="Auto Fill Locomotion Animation Set With Filters"))
	static int32 AutoFillLocomotionAnimationSetWithFilters(
		ULastFPSLocomotionAnimationSet* AnimationSet,
		const FString& ContentPath,
		const FString& RequiredNameText,
		const FString& RequiredPrefixText,
		bool bOverwriteExisting,
		bool bClearBeforeFill);
};
