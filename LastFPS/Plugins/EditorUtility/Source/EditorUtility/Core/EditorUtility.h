#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FEditorUtilityModule : public IModuleInterface
{
public:
	static const FName RuntimeStatsEditorTabName;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static void OpenRuntimeStatsEditor();
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnExtendLastFPSMenu, class FMenuBuilder&);
	static EDITORUTILITY_API FOnExtendLastFPSMenu& OnExtendLastFPSMenu();

private:
	void RegisterMenus();
	void RegisterTabSpawner();
	void UnregisterTabSpawner();

	void FillLastFPSMenu(class FMenuBuilder& MenuBuilder);
	TSharedRef<class SDockTab> OnSpawnLevelSelectionTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> OnSpawnCharacterDataAssetTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> OnSpawnRuntimeStatsEditorTab(const class FSpawnTabArgs& Args);
	void OpenLevelSelectionTool();
	
	void OpenCharacterDataAssetTool();
	void OpenRuntimeStatsTool();
	
	static const FName LevelSelectionTabName;
	static const FName CharacterDataAssetTabName;
	TSharedPtr<class FSlateStyleSet> StyleSetInstance;
};
