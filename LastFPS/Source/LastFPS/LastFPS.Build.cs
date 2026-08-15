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
			"CommonUI", "CommonInput", "CommonGame", "CommonUser", "ModularGameplayActors",
			"AIModule", "NavigationSystem", "Niagara", "EasyCrosshairSystem", "Synthesis",
			// 퀘스트 컷신 재생 (LastFPSCinematicPlaybackSubsystem)
			"LevelSequence", "MovieScene"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"NetCore", "Slate", "SlateCore", "CommonLoadingScreen", "RenderCore", "RHI",
			"OnlineBase", "OnlineSubsystem",
			// 마스터 로비 방 등록/하트비트용 Online Beacon
			"OnlineSubsystemUtils"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
