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
	void OpenLevelSelectionTool();

	static const FName LevelSelectionTabName;
	TSharedPtr<class FSlateStyleSet> StyleSetInstance;
};
