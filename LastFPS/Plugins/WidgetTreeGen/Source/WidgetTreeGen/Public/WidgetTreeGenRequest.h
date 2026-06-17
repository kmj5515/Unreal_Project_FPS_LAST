// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "WidgetTreeGenTypes.h"
#include "WidgetTreeGenRequest.generated.h"

class UUserWidget;

/**
 * A small settings object exposing a "Generate" button (CallInEditor) in a details panel.
 * Open it via the editor menu: Tools > Widget Tree Gen > Open Generator.
 * It can also be created and driven from Blueprint / Editor Utility Widgets.
 */
UCLASS(BlueprintType)
class WIDGETTREEGEN_API UWidgetTreeGenRequest : public UObject
{
	GENERATED_BODY()

public:
	/** Parent class of the generated Widget Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TSubclassOf<UUserWidget> ParentClass;

	/** When true, read the JSON from JsonFilePath instead of the inline JsonText. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bReadFromFile = false;

	/** Inline JSON document (used when bReadFromFile is false). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input",
		meta = (MultiLine = "true", EditCondition = "!bReadFromFile"))
	FString JsonText;

	/** Path to a .json file on disk (used when bReadFromFile is true). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input",
		meta = (FilePathFilter = "json", EditCondition = "bReadFromFile"))
	FString JsonFilePath;

	/** Content save path. Defaults to /Game/UI. If cleared, the JSON "savePath" (or /Game/UI) is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FString SavePath = TEXT("/Game/UI");

	/** Optional override for the asset name (e.g. WBP_Generated). Falls back to JSON. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FString AssetName;

	/** Result of the last Generate() call. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
	FWidgetTreeGenResult LastResult;

	/** Build the Widget Blueprint from the configured input. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "WidgetTreeGen")
	FWidgetTreeGenResult Generate();
};
