// Copyright Epic Games, Inc. All Rights Reserved.

#include "WidgetTreeGenRequest.h"
#include "WidgetTreeGenerator.h"

FWidgetTreeGenResult UWidgetTreeGenRequest::Generate()
{
	if (bReadFromFile)
	{
		LastResult = FWidgetTreeGenerator::GenerateFromJsonFile(JsonFilePath, ParentClass, SavePath, AssetName, bOverwriteExisting);
	}
	else
	{
		LastResult = FWidgetTreeGenerator::GenerateFromJsonString(JsonText, ParentClass, SavePath, AssetName, bOverwriteExisting);
	}
	return LastResult;
}
