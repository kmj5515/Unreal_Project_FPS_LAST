using UnrealBuildTool;

public class LastFPSEditor : ModuleRules
{
	public LastFPSEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"LastFPS"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"ImageCore",
			"RenderCore",
			"UnrealEd"
		});
	}
}
