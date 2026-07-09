#pragma once

#include "BattleLevelTool/LastFPSBattleLevelService.h"
#include "Styling/SlateColor.h"
#include "Types/SlateEnums.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;
class STextBlock;

class SLastFPSBattleLevelTool : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLastFPSBattleLevelTool) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void RefreshBattleLevels();
	void RefreshLevelList();
	void RefreshMonsterDraftList();
	void RefreshValidationMessages();
	FReply RefreshClicked();
	FReply ValidateCurrentClicked();
	FReply AddMonsterClicked();
	FReply ClearMonstersClicked();
	FReply CreateScenarioClicked();
	FReply SaveScenarioClicked();
	FReply PlayScenarioClicked();
	FReply LoadSelectedScenarioClicked();
	FReply PlaySelectedScenarioClicked();
	FReply OpenLevelClicked(FLastFPSBattleLevelInfo BattleLevel);
	FReply PlayLevelClicked(FLastFPSBattleLevelInfo BattleLevel);
	FReply UseLevelForScenarioClicked(FLastFPSBattleLevelInfo BattleLevel);
	TSharedRef<SWidget> BuildScenarioEditor();
	TSharedRef<SWidget> BuildLevelRow(const FLastFPSBattleLevelInfo& BattleLevel);
	TSharedRef<SWidget> BuildValidationRow(const FLastFPSBattleLevelValidationMessage& Message) const;
	FSlateColor GetSeverityColor(ELastFPSBattleLevelValidationSeverity Severity) const;
	FText GetCurrentLevelText() const;
	FText GetScenarioNameText() const;
	FText GetPendingMonsterSpawnTagText() const;
	FString GetSelectedScenarioObjectPath() const;
	FString GetBattleMapObjectPath() const;
	FString GetPlayerCharacterObjectPath() const;
	FString GetPendingMonsterObjectPath() const;
	void SetSelectedScenarioObject(const struct FAssetData& AssetData);
	void SetBattleMapObject(const struct FAssetData& AssetData);
	void SetPlayerCharacterObject(const struct FAssetData& AssetData);
	void SetPendingMonsterObject(const struct FAssetData& AssetData);
	void SetScenarioName(const FText& NewText, ETextCommit::Type CommitType);
	void SetPendingMonsterSpawnTag(const FText& NewText, ETextCommit::Type CommitType);
	void SetStatus(const FText& NewStatus);

	TArray<FLastFPSBattleLevelInfo> BattleLevels;
	TArray<FLastFPSBattleScenarioMonsterEntry> DraftMonsters;
	TArray<FLastFPSBattleLevelValidationMessage> ValidationMessages;
	FString SelectedScenarioObjectPath;
	FString ScenarioName = TEXT("NewScenario");
	FString BattleMapObjectPath;
	FString PlayerCharacterObjectPath;
	FString PendingMonsterObjectPath;
	int32 PendingMonsterCount = 1;
	FName PendingMonsterSpawnTag = TEXT("EnemySpawn");
	float PendingMonsterLevelScale = 1.f;
	TSharedPtr<SScrollBox> LevelListBox;
	TSharedPtr<SScrollBox> MonsterDraftBox;
	TSharedPtr<SScrollBox> ValidationBox;
	TSharedPtr<STextBlock> StatusText;
};
