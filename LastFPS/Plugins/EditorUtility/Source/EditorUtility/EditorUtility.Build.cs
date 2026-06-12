// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EditorUtility : ModuleRules
{
	public EditorUtility(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"UMG",
				"Blutility",
				"AssetRegistry",
				"Json",
				"JsonUtilities",
				"DeveloperSettings",
				"LastFPS"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"UnrealEd",
				"UMGEditor",
				"ToolMenus",
				"DesktopPlatform",
				"EngineSettings",
				"AssetTools",
				"ContentBrowser",
				"PropertyEditor"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
