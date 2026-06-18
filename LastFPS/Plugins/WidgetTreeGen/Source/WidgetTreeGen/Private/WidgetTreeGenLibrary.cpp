// Copyright Epic Games, Inc. All Rights Reserved.

#include "WidgetTreeGenLibrary.h"
#include "WidgetTreeGenerator.h"

FWidgetTreeGenResult UWidgetTreeGenLibrary::GenerateFromJsonText(
	const FString& JsonText,
	TSubclassOf<UUserWidget> ParentClass,
	const FString& SavePath,
	const FString& AssetName)
{
	return FWidgetTreeGenerator::GenerateFromJsonString(JsonText, ParentClass, SavePath, AssetName);
}

FWidgetTreeGenResult UWidgetTreeGenLibrary::GenerateFromJsonFile(
	const FString& JsonFilePath,
	TSubclassOf<UUserWidget> ParentClass,
	const FString& SavePath,
	const FString& AssetName)
{
	return FWidgetTreeGenerator::GenerateFromJsonFile(JsonFilePath, ParentClass, SavePath, AssetName);
}
