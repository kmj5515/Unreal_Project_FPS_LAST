// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FEditorUtilityModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void RegisterTabSpawner();
	void UnregisterTabSpawner();

	void FillLastFPSMenu(class FMenuBuilder& MenuBuilder);
	TSharedRef<class SDockTab> OnSpawnLevelSelectionTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> OnSpawnCharacterDataAssetTab(const class FSpawnTabArgs& Args);
	void OpenLevelSelectionTool();
	
	void OpenCharacterDataAssetTool();
	
	static const FName LevelSelectionTabName;
	static const FName CharacterDataAssetTabName;
	TSharedPtr<class FSlateStyleSet> StyleSetInstance;
};
