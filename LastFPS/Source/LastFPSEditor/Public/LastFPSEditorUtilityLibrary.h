#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LastFPSEditorUtilityLibrary.generated.h"

UCLASS()
class LASTFPSEDITOR_API ULastFPSEditorUtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LastFPS|Editor Utility")
	static void OpenRuntimeStatsEditor();
};
