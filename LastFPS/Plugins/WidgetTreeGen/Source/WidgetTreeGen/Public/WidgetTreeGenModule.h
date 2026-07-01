// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FWidgetTreeGenModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Appends our entries to the EditorUtility "LastFPS" submenu (bound to its extension delegate). */
	void ExtendLastFPSMenu(class FMenuBuilder& MenuBuilder);

	/** Opens the generator details window. */
	void OpenGeneratorWindow();

	/** Opens a file picker and generates a Widget Blueprint from the chosen JSON file. */
	void GenerateFromFileDialog();

	/** Opens a folder picker and generates from every *.json in it (overwrite honored per file). */
	void GenerateFromFolderDialog();

	/** Generates BP_<RowName> NPC presets from the DT_NPCData configured in Project Settings. */
	void GenerateNPCPresets();
};
