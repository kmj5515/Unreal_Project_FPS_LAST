#pragma once

#include "CoreMinimal.h"
#include "BattleLevelTool/LastFPSBattleScenarioDefinition.h"

class UWorld;

enum class ELastFPSBattleLevelValidationSeverity : uint8
{
	Info,
	Warning,
	Error
};

struct FLastFPSBattleLevelInfo
{
	FString DisplayName;
	FString PackageName;
	FSoftObjectPath AssetPath;
};

struct FLastFPSBattleLevelValidationMessage
{
	ELastFPSBattleLevelValidationSeverity Severity = ELastFPSBattleLevelValidationSeverity::Info;
	FText Message;
};

class FLastFPSBattleLevelService
{
public:
	static const FString& GetScenarioRootPath();
	static void CollectBattleLevels(TArray<FLastFPSBattleLevelInfo>& OutBattleLevels);
	static bool OpenBattleLevel(const FLastFPSBattleLevelInfo& BattleLevel);
	static bool PlayBattleLevel(const FLastFPSBattleLevelInfo& BattleLevel);
	static bool PlayScenario(
		const FSoftObjectPath& BattleMapPath,
		const FSoftObjectPath& PlayerCharacterPath,
		const TArray<FLastFPSBattleScenarioMonsterEntry>& Monsters,
		FText& OutErrorMessage);
	static bool PlayScenarioAsset(
		const FSoftObjectPath& ScenarioPath,
		FText& OutErrorMessage);
	static bool LoadScenarioAsset(
		const FSoftObjectPath& ScenarioPath,
		FString& OutScenarioName,
		FString& OutBattleMapObjectPath,
		FString& OutPlayerCharacterObjectPath,
		TArray<FLastFPSBattleScenarioMonsterEntry>& OutMonsters,
		FText& OutErrorMessage);
	static bool CreateScenarioAsset(
		const FString& ScenarioName,
		const FSoftObjectPath& BattleMapPath,
		const FSoftObjectPath& PlayerCharacterPath,
		const TArray<FLastFPSBattleScenarioMonsterEntry>& Monsters,
		FString& OutScenarioObjectPath,
		FText& OutErrorMessage);
	static bool SaveScenarioAsset(
		const FSoftObjectPath& ScenarioPath,
		const FString& ScenarioName,
		const FSoftObjectPath& BattleMapPath,
		const FSoftObjectPath& PlayerCharacterPath,
		const TArray<FLastFPSBattleScenarioMonsterEntry>& Monsters,
		FString& OutScenarioObjectPath,
		FText& OutErrorMessage);
	static void ValidateCurrentBattleLevel(TArray<FLastFPSBattleLevelValidationMessage>& OutMessages);
	static FString GetCurrentLevelPackageName();

private:
	static FString GetBattleLevelRootPath();
	static bool IsPackageUsable(const FString& PackageName);
	static bool IsBattleLevelPackage(const FString& PackageName);
	static bool HasActorWithTag(UWorld& World, FName TagName);
	static void AddMessage(TArray<FLastFPSBattleLevelValidationMessage>& OutMessages, ELastFPSBattleLevelValidationSeverity Severity, const FText& Message);
};
