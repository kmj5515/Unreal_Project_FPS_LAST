// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LastFPS : ModuleRules
{
	public LastFPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 모듈 루트를 include path에 추가 → "AbilitySystem/AttributeSets/Foo.h" 형태로 사용 가능
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			"UMG", "Json", "JsonUtilities", "AssetRegistry", "DeveloperSettings",
			"CommonUI", "CommonInput", "CommonGame", "ModularGameplayActors",
			"AIModule", "NavigationSystem", "Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "NetCore", "Slate", "SlateCore", "CommonLoadingScreen" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
