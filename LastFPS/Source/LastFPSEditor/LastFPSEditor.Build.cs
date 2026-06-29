using UnrealBuildTool;

public class LastFPSEditor : ModuleRules
{
	public LastFPSEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"LastFPS"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"ApplicationCore",
			"GameplayAbilities",
			"InputCore",
			"Slate",
			"SlateCore",
			"UnrealEd"
		});
	}
}
