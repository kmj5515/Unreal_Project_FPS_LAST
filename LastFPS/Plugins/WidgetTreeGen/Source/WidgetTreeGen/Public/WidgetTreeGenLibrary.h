// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "WidgetTreeGenTypes.h"
#include "WidgetTreeGenLibrary.generated.h"

class UUserWidget;

/**
 * Blueprint entry points for the Widget Tree Generator.
 * Designed to be called from an Editor Utility Widget (Blutility).
 */
UCLASS()
class WIDGETTREEGEN_API UWidgetTreeGenLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Generate a UMG Widget Blueprint from a JSON string.
	 *
	 * @param JsonText      The JSON document describing the widget hierarchy.
	 * @param ParentClass   Parent UUserWidget class. If null, the JSON "parentClass" field (or UUserWidget) is used.
	 * @param SavePath      Content path, e.g. /Game/UI/Generated. If empty, the JSON "savePath" field is used.
	 * @param AssetName     Asset name, e.g. WBP_Generated. If empty, the JSON "assetName" field is used.
	 */
	UFUNCTION(BlueprintCallable, Category = "WidgetTreeGen",
		meta = (DisplayName = "Generate Widget Blueprint From JSON Text"))
	static FWidgetTreeGenResult GenerateFromJsonText(
		const FString& JsonText,
		TSubclassOf<UUserWidget> ParentClass,
		const FString& SavePath,
		const FString& AssetName);

	/**
	 * Generate a UMG Widget Blueprint from a JSON file on disk.
	 *
	 * @param JsonFilePath  Absolute path to a .json file.
	 */
	UFUNCTION(BlueprintCallable, Category = "WidgetTreeGen",
		meta = (DisplayName = "Generate Widget Blueprint From JSON File"))
	static FWidgetTreeGenResult GenerateFromJsonFile(
		const FString& JsonFilePath,
		TSubclassOf<UUserWidget> ParentClass,
		const FString& SavePath,
		const FString& AssetName);
};
