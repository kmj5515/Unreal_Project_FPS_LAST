// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WidgetTreeGenTypes.generated.h"

/**
 * Result of a generation request, returned to Blueprint / Editor Utility Widgets.
 */
USTRUCT(BlueprintType)
struct FWidgetTreeGenResult
{
	GENERATED_BODY()

	/** True if the widget blueprint was created, compiled and saved without errors. */
	UPROPERTY(BlueprintReadOnly, Category = "WidgetTreeGen")
	bool bSuccess = false;

	/** Object path of the generated asset (e.g. /Game/UI/Generated/WBP_Generated). Empty on failure. */
	UPROPERTY(BlueprintReadOnly, Category = "WidgetTreeGen")
	FString AssetPath;

	/** Human-readable error message when bSuccess is false. */
	UPROPERTY(BlueprintReadOnly, Category = "WidgetTreeGen")
	FString ErrorMessage;

	static FWidgetTreeGenResult MakeError(const FString& InMessage)
	{
		FWidgetTreeGenResult Result;
		Result.bSuccess = false;
		Result.ErrorMessage = InMessage;
		return Result;
	}

	static FWidgetTreeGenResult MakeSuccess(const FString& InAssetPath)
	{
		FWidgetTreeGenResult Result;
		Result.bSuccess = true;
		Result.AssetPath = InAssetPath;
		return Result;
	}
};
