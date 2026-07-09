#include "BattleLevelTool/SLastFPSBattleLevelTool.h"

#include "AssetRegistry/AssetData.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Engine/World.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SLastFPSBattleLevelTool"

void SLastFPSBattleLevelTool::Construct(const FArguments& InArgs)
{
	RefreshBattleLevels();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Title", "Battle Level Tool"))
				.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 14))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Refresh", "Refresh"))
					.OnClicked(this, &SLastFPSBattleLevelTool::RefreshClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ValidateCurrent", "Validate Current"))
					.OnClicked(this, &SLastFPSBattleLevelTool::ValidateCurrentClicked)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SLastFPSBattleLevelTool::GetCurrentLevelText)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildScenarioEditor()
			]
			+ SVerticalBox::Slot()
			.FillHeight(0.52f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(8.f)
				[
					SAssignNew(LevelListBox, SScrollBox)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(0.28f)
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(8.f)
				[
					SAssignNew(ValidationBox, SScrollBox)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SAssignNew(StatusText, STextBlock)
				.Text(LOCTEXT("Ready", "Ready."))
			]
		]
	];

	RefreshLevelList();
	RefreshMonsterDraftList();
	RefreshValidationMessages();
}

void SLastFPSBattleLevelTool::RefreshBattleLevels()
{
	FLastFPSBattleLevelService::CollectBattleLevels(BattleLevels);
}

void SLastFPSBattleLevelTool::RefreshLevelList()
{
	if (!LevelListBox)
	{
		return;
	}

	LevelListBox->ClearChildren();

	if (BattleLevels.IsEmpty())
	{
		LevelListBox->AddSlot()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoLevels", "No battle levels found. Check MWTool > Battle Levels settings."))
		];
		return;
	}

	for (const FLastFPSBattleLevelInfo& BattleLevel : BattleLevels)
	{
		LevelListBox->AddSlot()
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			BuildLevelRow(BattleLevel)
		];
	}
}

void SLastFPSBattleLevelTool::RefreshMonsterDraftList()
{
	if (!MonsterDraftBox)
	{
		return;
	}

	MonsterDraftBox->ClearChildren();

	if (DraftMonsters.IsEmpty())
	{
		MonsterDraftBox->AddSlot()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoDraftMonsters", "No monsters added."))
		];
		return;
	}

	for (int32 MonsterIndex = 0; MonsterIndex < DraftMonsters.Num(); ++MonsterIndex)
	{
		const FLastFPSBattleScenarioMonsterEntry& Monster = DraftMonsters[MonsterIndex];
		MonsterDraftBox->AddSlot()
		.Padding(0.f, 0.f, 0.f, 3.f)
		[
			SNew(STextBlock)
			.Text(FText::Format(
				LOCTEXT("DraftMonsterRow", "{0}. {1} x{2} / SpawnTag: {3} / Scale: {4}"),
				FText::AsNumber(MonsterIndex + 1),
				FText::FromString(Monster.MonsterDefinition.ToSoftObjectPath().GetAssetName()),
				FText::AsNumber(Monster.Count),
				FText::FromName(Monster.SpawnTag),
				FText::AsNumber(Monster.LevelScale)
			))
		];
	}
}

void SLastFPSBattleLevelTool::RefreshValidationMessages()
{
	if (!ValidationBox)
	{
		return;
	}

	ValidationBox->ClearChildren();

	if (ValidationMessages.IsEmpty())
	{
		ValidationBox->AddSlot()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoValidation", "Press Validate Current to inspect the current battle level."))
		];
		return;
	}

	for (const FLastFPSBattleLevelValidationMessage& Message : ValidationMessages)
	{
		ValidationBox->AddSlot()
		.Padding(0.f, 0.f, 0.f, 3.f)
		[
			BuildValidationRow(Message)
		];
	}
}

FReply SLastFPSBattleLevelTool::RefreshClicked()
{
	RefreshBattleLevels();
	RefreshLevelList();
	SetStatus(FText::Format(LOCTEXT("RefreshedStatus", "Loaded {0} battle levels."), FText::AsNumber(BattleLevels.Num())));
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::ValidateCurrentClicked()
{
	FLastFPSBattleLevelService::ValidateCurrentBattleLevel(ValidationMessages);
	RefreshValidationMessages();
	SetStatus(LOCTEXT("ValidatedStatus", "Current level validation finished."));
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::AddMonsterClicked()
{
	const FSoftObjectPath MonsterPath(PendingMonsterObjectPath);
	if (!MonsterPath.IsValid())
	{
		SetStatus(LOCTEXT("MonsterMissingStatus", "Select a monster definition first."));
		return FReply::Handled();
	}

	FLastFPSBattleScenarioMonsterEntry MonsterEntry;
	MonsterEntry.MonsterDefinition = TSoftObjectPtr<ULastFPSCharacterDefinition>(MonsterPath);
	MonsterEntry.Count = FMath::Max(PendingMonsterCount, 1);
	MonsterEntry.SpawnTag = PendingMonsterSpawnTag.IsNone() ? FName(TEXT("EnemySpawn")) : PendingMonsterSpawnTag;
	MonsterEntry.LevelScale = FMath::Max(PendingMonsterLevelScale, 0.01f);

	DraftMonsters.Add(MonsterEntry);
	RefreshMonsterDraftList();
	SetStatus(LOCTEXT("MonsterAddedStatus", "Monster entry added."));
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::ClearMonstersClicked()
{
	DraftMonsters.Reset();
	RefreshMonsterDraftList();
	SetStatus(LOCTEXT("MonstersClearedStatus", "Monster entries cleared."));
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::CreateScenarioClicked()
{
	FString SavedObjectPath;
	FText ErrorMessage;
	if (FLastFPSBattleLevelService::CreateScenarioAsset(
		ScenarioName,
		FSoftObjectPath(BattleMapObjectPath),
		FSoftObjectPath(PlayerCharacterObjectPath),
		DraftMonsters,
		SavedObjectPath,
		ErrorMessage))
	{
		SelectedScenarioObjectPath = SavedObjectPath;
		SetStatus(FText::Format(LOCTEXT("ScenarioCreatedStatus", "Created scenario: {0}"), FText::FromString(SavedObjectPath)));
		return FReply::Handled();
	}

	SetStatus(ErrorMessage);
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::SaveScenarioClicked()
{
	FString SavedObjectPath;
	FText ErrorMessage;
	if (FLastFPSBattleLevelService::SaveScenarioAsset(
		FSoftObjectPath(SelectedScenarioObjectPath),
		ScenarioName,
		FSoftObjectPath(BattleMapObjectPath),
		FSoftObjectPath(PlayerCharacterObjectPath),
		DraftMonsters,
		SavedObjectPath,
		ErrorMessage))
	{
		SelectedScenarioObjectPath = SavedObjectPath;
		SetStatus(FText::Format(LOCTEXT("ScenarioUpdatedStatus", "Updated scenario: {0}"), FText::FromString(SavedObjectPath)));
		return FReply::Handled();
	}

	SetStatus(ErrorMessage);
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::PlayScenarioClicked()
{
	FText ErrorMessage;
	if (FLastFPSBattleLevelService::PlayScenario(
		FSoftObjectPath(BattleMapObjectPath),
		FSoftObjectPath(PlayerCharacterObjectPath),
		DraftMonsters,
		ErrorMessage))
	{
		SetStatus(LOCTEXT("ScenarioPlayQueuedStatus", "Queued PIE with current scenario draft."));
		return FReply::Handled();
	}

	SetStatus(ErrorMessage);
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::LoadSelectedScenarioClicked()
{
	FText ErrorMessage;
	if (FLastFPSBattleLevelService::LoadScenarioAsset(
		FSoftObjectPath(SelectedScenarioObjectPath),
		ScenarioName,
		BattleMapObjectPath,
		PlayerCharacterObjectPath,
		DraftMonsters,
		ErrorMessage))
	{
		RefreshMonsterDraftList();
		SetStatus(LOCTEXT("ScenarioLoadedStatus", "Loaded selected scenario into draft."));
		return FReply::Handled();
	}

	SetStatus(ErrorMessage);
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::PlaySelectedScenarioClicked()
{
	FText ErrorMessage;
	if (FLastFPSBattleLevelService::PlayScenarioAsset(
		FSoftObjectPath(SelectedScenarioObjectPath),
		ErrorMessage))
	{
		SetStatus(LOCTEXT("SelectedScenarioPlayQueuedStatus", "Queued PIE with selected scenario."));
		return FReply::Handled();
	}

	SetStatus(ErrorMessage);
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::OpenLevelClicked(FLastFPSBattleLevelInfo BattleLevel)
{
	if (FLastFPSBattleLevelService::OpenBattleLevel(BattleLevel))
	{
		SetStatus(FText::Format(LOCTEXT("OpenedStatus", "Opened {0}."), FText::FromString(BattleLevel.DisplayName)));
	}

	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::PlayLevelClicked(FLastFPSBattleLevelInfo BattleLevel)
{
	if (FLastFPSBattleLevelService::PlayBattleLevel(BattleLevel))
	{
		SetStatus(FText::Format(LOCTEXT("PlayStatus", "Started PIE with {0}."), FText::FromString(BattleLevel.DisplayName)));
	}

	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::UseLevelForScenarioClicked(FLastFPSBattleLevelInfo BattleLevel)
{
	BattleMapObjectPath = BattleLevel.AssetPath.ToString();
	SetStatus(FText::Format(LOCTEXT("ScenarioMapSelectedStatus", "Scenario map set to {0}."), FText::FromString(BattleLevel.DisplayName)));
	return FReply::Handled();
}

TSharedRef<SWidget> SLastFPSBattleLevelTool::BuildScenarioEditor()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ScenarioEditorTitle", "Scenario Draft"))
				.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 11))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(ULastFPSBattleScenarioDefinition::StaticClass())
					.ObjectPath(this, &SLastFPSBattleLevelTool::GetSelectedScenarioObjectPath)
					.OnObjectChanged(this, &SLastFPSBattleLevelTool::SetSelectedScenarioObject)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("LoadSelectedScenario", "Load Selected"))
					.OnClicked(this, &SLastFPSBattleLevelTool::LoadSelectedScenarioClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("PlaySelectedScenario", "Play Selected"))
					.OnClicked(this, &SLastFPSBattleLevelTool::PlaySelectedScenarioClicked)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.25f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SEditableTextBox)
					.Text(this, &SLastFPSBattleLevelTool::GetScenarioNameText)
					.OnTextCommitted(this, &SLastFPSBattleLevelTool::SetScenarioName)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.25f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UWorld::StaticClass())
					.ObjectPath(this, &SLastFPSBattleLevelTool::GetBattleMapObjectPath)
					.OnObjectChanged(this, &SLastFPSBattleLevelTool::SetBattleMapObject)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.25f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(ULastFPSCharacterDefinition::StaticClass())
					.ObjectPath(this, &SLastFPSBattleLevelTool::GetPlayerCharacterObjectPath)
					.OnObjectChanged(this, &SLastFPSBattleLevelTool::SetPlayerCharacterObject)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("CreateScenario", "Create Scenario"))
					.OnClicked(this, &SLastFPSBattleLevelTool::CreateScenarioClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("SaveScenario", "Save Selected"))
					.OnClicked(this, &SLastFPSBattleLevelTool::SaveScenarioClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("PlayScenario", "Play Scenario"))
					.OnClicked(this, &SLastFPSBattleLevelTool::PlayScenarioClicked)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(ULastFPSCharacterDefinition::StaticClass())
					.ObjectPath(this, &SLastFPSBattleLevelTool::GetPendingMonsterObjectPath)
					.OnObjectChanged(this, &SLastFPSBattleLevelTool::SetPendingMonsterObject)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SSpinBox<int32>)
					.MinValue(1)
					.MaxValue(999)
					.Value_Lambda([this]() { return PendingMonsterCount; })
					.OnValueChanged_Lambda([this](int32 NewValue) { PendingMonsterCount = FMath::Max(NewValue, 1); })
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.22f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SEditableTextBox)
					.Text(this, &SLastFPSBattleLevelTool::GetPendingMonsterSpawnTagText)
					.OnTextCommitted(this, &SLastFPSBattleLevelTool::SetPendingMonsterSpawnTag)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SSpinBox<float>)
					.MinValue(0.01f)
					.MaxValue(100.f)
					.Value_Lambda([this]() { return PendingMonsterLevelScale; })
					.OnValueChanged_Lambda([this](float NewValue) { PendingMonsterLevelScale = FMath::Max(NewValue, 0.01f); })
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("AddMonster", "Add Monster"))
					.OnClicked(this, &SLastFPSBattleLevelTool::AddMonsterClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("ClearMonsters", "Clear"))
					.OnClicked(this, &SLastFPSBattleLevelTool::ClearMonstersClicked)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SAssignNew(MonsterDraftBox, SScrollBox)
			]
		];
}

TSharedRef<SWidget> SLastFPSBattleLevelTool::BuildLevelRow(const FLastFPSBattleLevelInfo& BattleLevel)
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(BattleLevel.DisplayName))
					.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 10))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(BattleLevel.PackageName))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenLevel", "Open"))
				.OnClicked(this, &SLastFPSBattleLevelTool::OpenLevelClicked, BattleLevel)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(LOCTEXT("PlayLevel", "Play"))
				.OnClicked(this, &SLastFPSBattleLevelTool::PlayLevelClicked, BattleLevel)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(LOCTEXT("UseLevelForScenario", "Use"))
				.OnClicked(this, &SLastFPSBattleLevelTool::UseLevelForScenarioClicked, BattleLevel)
			]
		];
}

TSharedRef<SWidget> SLastFPSBattleLevelTool::BuildValidationRow(const FLastFPSBattleLevelValidationMessage& Message) const
{
	return SNew(STextBlock)
		.Text(Message.Message)
		.ColorAndOpacity(GetSeverityColor(Message.Severity));
}

FSlateColor SLastFPSBattleLevelTool::GetSeverityColor(ELastFPSBattleLevelValidationSeverity Severity) const
{
	switch (Severity)
	{
	case ELastFPSBattleLevelValidationSeverity::Error:
		return FSlateColor(FLinearColor(1.f, 0.2f, 0.2f));
	case ELastFPSBattleLevelValidationSeverity::Warning:
		return FSlateColor(FLinearColor(1.f, 0.72f, 0.16f));
	default:
		return FSlateColor(FLinearColor(0.55f, 0.9f, 0.55f));
	}
}

FText SLastFPSBattleLevelTool::GetCurrentLevelText() const
{
	const FString CurrentPackageName = FLastFPSBattleLevelService::GetCurrentLevelPackageName();
	return CurrentPackageName.IsEmpty()
		? LOCTEXT("NoCurrentLevelText", "Current: None")
		: FText::Format(LOCTEXT("CurrentLevelText", "Current: {0}"), FText::FromString(CurrentPackageName));
}

FText SLastFPSBattleLevelTool::GetScenarioNameText() const
{
	return FText::FromString(ScenarioName);
}

FText SLastFPSBattleLevelTool::GetPendingMonsterSpawnTagText() const
{
	return FText::FromName(PendingMonsterSpawnTag);
}

FString SLastFPSBattleLevelTool::GetBattleMapObjectPath() const
{
	return BattleMapObjectPath;
}

FString SLastFPSBattleLevelTool::GetSelectedScenarioObjectPath() const
{
	return SelectedScenarioObjectPath;
}

FString SLastFPSBattleLevelTool::GetPlayerCharacterObjectPath() const
{
	return PlayerCharacterObjectPath;
}

FString SLastFPSBattleLevelTool::GetPendingMonsterObjectPath() const
{
	return PendingMonsterObjectPath;
}

void SLastFPSBattleLevelTool::SetSelectedScenarioObject(const FAssetData& AssetData)
{
	SelectedScenarioObjectPath = AssetData.IsValid() ? AssetData.GetSoftObjectPath().ToString() : FString();
}

void SLastFPSBattleLevelTool::SetBattleMapObject(const FAssetData& AssetData)
{
	BattleMapObjectPath = AssetData.IsValid() ? AssetData.GetSoftObjectPath().ToString() : FString();
}

void SLastFPSBattleLevelTool::SetPlayerCharacterObject(const FAssetData& AssetData)
{
	PlayerCharacterObjectPath = AssetData.IsValid() ? AssetData.GetSoftObjectPath().ToString() : FString();
}

void SLastFPSBattleLevelTool::SetPendingMonsterObject(const FAssetData& AssetData)
{
	PendingMonsterObjectPath = AssetData.IsValid() ? AssetData.GetSoftObjectPath().ToString() : FString();
}

void SLastFPSBattleLevelTool::SetScenarioName(const FText& NewText, ETextCommit::Type CommitType)
{
	ScenarioName = NewText.ToString();
}

void SLastFPSBattleLevelTool::SetPendingMonsterSpawnTag(const FText& NewText, ETextCommit::Type CommitType)
{
	PendingMonsterSpawnTag = FName(*NewText.ToString());
}

void SLastFPSBattleLevelTool::SetStatus(const FText& NewStatus)
{
	if (StatusText)
	{
		StatusText->SetText(NewStatus);
	}
}

#undef LOCTEXT_NAMESPACE
