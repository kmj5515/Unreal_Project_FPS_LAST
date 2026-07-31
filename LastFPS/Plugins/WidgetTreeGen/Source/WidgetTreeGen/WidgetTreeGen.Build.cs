// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WidgetTreeGen : ModuleRules
{
	public WidgetTreeGen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",
				"Json",
				"JsonUtilities"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"UnrealEd",
				"UMGEditor",
				"AssetTools",
				"Kismet",
				"EditorScriptingUtilities",
				"Blutility",
				"ToolMenus",
				"PropertyEditor",
				"DesktopPlatform",
				"EditorUtility",
				"LastFPS"
			}
		);
	}
}
