// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FEditorUtilityModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Extension point so other editor modules can append entries to the "LastFPS"
	 * main-menu submenu without this module depending on them.
	 * Bind from your module's StartupModule(); entries are added below the built-in ones.
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnExtendLastFPSMenu, class FMenuBuilder& /*MenuBuilder*/);
	static EDITORUTILITY_API FOnExtendLastFPSMenu& OnExtendLastFPSMenu();

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
