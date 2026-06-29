#include "LastFPSEditorModule.h"

#include "SLastFPSRuntimeStatsEditor.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "LastFPSEditor"

const FName FLastFPSEditorModule::RuntimeStatsEditorTabName(TEXT("LastFPS.RuntimeStatsEditor"));

void FLastFPSEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		RuntimeStatsEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FLastFPSEditorModule::SpawnRuntimeStatsEditorTab))
		.SetDisplayName(LOCTEXT("RuntimeStatsEditorTab", "LastFPS Runtime Stats"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FLastFPSEditorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RuntimeStatsEditorTabName);
}

void FLastFPSEditorModule::OpenRuntimeStatsEditor()
{
	FGlobalTabmanager::Get()->TryInvokeTab(RuntimeStatsEditorTabName);
}

TSharedRef<SDockTab> FLastFPSEditorModule::SpawnRuntimeStatsEditorTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SLastFPSRuntimeStatsEditor)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLastFPSEditorModule, LastFPSEditor)
