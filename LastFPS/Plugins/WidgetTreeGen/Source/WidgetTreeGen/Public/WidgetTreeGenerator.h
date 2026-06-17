// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "WidgetTreeGenTypes.h"

class UUserWidget;

/**
 * Core, UObject-free generator: parses a JSON hierarchy and builds / compiles / saves
 * a UWidgetBlueprint. Editor-only (relies on UMGEditor + UnrealEd).
 */
class WIDGETTREEGEN_API FWidgetTreeGenerator
{
public:
	/**
	 * Generate a Widget Blueprint from a JSON string.
	 *
	 * @param JsonString       The full JSON document (see README for schema).
	 * @param ParentClassOverride  When valid, overrides the JSON "parentClass" field.
	 * @param SavePathOverride     When non-empty, overrides the JSON "savePath" field.
	 * @param AssetNameOverride    When non-empty, overrides the JSON "assetName" field.
	 */
	static FWidgetTreeGenResult GenerateFromJsonString(
		const FString& JsonString,
		TSubclassOf<UUserWidget> ParentClassOverride = nullptr,
		const FString& SavePathOverride = FString(),
		const FString& AssetNameOverride = FString());

	/** Same as above but reads the JSON document from a file on disk. */
	static FWidgetTreeGenResult GenerateFromJsonFile(
		const FString& JsonFilePath,
		TSubclassOf<UUserWidget> ParentClassOverride = nullptr,
		const FString& SavePathOverride = FString(),
		const FString& AssetNameOverride = FString());

	/**
	 * Resolve a widget type string to a UClass.
	 * Tries the built-in short-name map first ("CanvasPanel", "Button", ...),
	 * then falls back to a full object path load ("/Script/UMG.Button",
	 * "/Game/UI/WBP_Foo.WBP_Foo_C"). Never uses ANY_PACKAGE.
	 */
	static UClass* ResolveWidgetClass(const FString& TypeString);
};
