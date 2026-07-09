using UnrealBuildTool;
using System.IO;

public class EditorUtility : ModuleRules
{
	public EditorUtility(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				ModuleDirectory,
				Path.Combine(ModuleDirectory, "Core")
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				ModuleDirectory,
				Path.Combine(ModuleDirectory, "BattleLevelTool"),
				Path.Combine(ModuleDirectory, "CharacterDatatAssetTool"),
				Path.Combine(ModuleDirectory, "Core"),
				Path.Combine(ModuleDirectory, "EditorPlay"),
				Path.Combine(ModuleDirectory, "LevelSelection"),
				Path.Combine(ModuleDirectory, "RuntimeStats"),
				Path.Combine(ModuleDirectory, "Settings")
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
				"ApplicationCore",
				"GameplayAbilities",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"Settings",
				"UMGEditor",
				"ToolMenus",
				"DesktopPlatform",
				"EngineSettings",
				"LevelEditor",
				"AssetTools",
				"ContentBrowser",
				"PropertyEditor"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}
