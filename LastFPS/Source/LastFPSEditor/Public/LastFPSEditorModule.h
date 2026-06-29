#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FLastFPSEditorModule : public IModuleInterface
{
public:
	static const FName RuntimeStatsEditorTabName;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static void OpenRuntimeStatsEditor();

private:
	TSharedRef<class SDockTab> SpawnRuntimeStatsEditorTab(const class FSpawnTabArgs& Args);
	void RegisterMenus();
};
