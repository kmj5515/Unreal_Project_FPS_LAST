#include "BattleLevelTool/SLastFPSBattleLevelTool.h"

#include "LastFPSEditorWidgets.h"
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
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SLastFPSBattleLevelTool"

void SLastFPSBattleLevelTool::Construct(const FArguments& InArgs)
{
	RefreshBattleLevels();

	ChildSlot
	[
		LastFPSEditorWidgets::MakeToolPanel(
			LOCTEXT("BattleToolPanelTitle", "배틀 레벨 도구"),
			LOCTEXT("BattleToolPanelSubtitle", "시나리오"),
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				LastFPSEditorWidgets::MakeFormRow(
					LOCTEXT("CurrentLevelRowLabel", "현재 레벨"),
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(SButton)
						.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
						.Text(LOCTEXT("Refresh", "새로고침"))
						.OnClicked(this, &SLastFPSBattleLevelTool::RefreshClicked)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(SButton)
						.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
						.Text(LOCTEXT("ValidateCurrent", "현재 레벨 검증"))
						.OnClicked(this, &SLastFPSBattleLevelTool::ValidateCurrentClicked)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SLastFPSBattleLevelTool::GetCurrentLevelText)
						.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
					])
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
				LastFPSEditorWidgets::MakeSection(
					LOCTEXT("BattleLevelsSection", "배틀 레벨"),
					LastFPSEditorWidgets::GetToolAccentColor(),
					SAssignNew(LevelListBox, SScrollBox))
			]

			+ SVerticalBox::Slot()
			.FillHeight(0.28f)
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				LastFPSEditorWidgets::MakeSection(
					LOCTEXT("ValidationSection", "검증"),
					LastFPSEditorWidgets::GetToolAccentColor(),
					SAssignNew(ValidationBox, SScrollBox),
					false)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SAssignNew(StatusText, STextBlock)
				.Text(LOCTEXT("Ready", "준비됨."))
				.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
			])
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
			.Text(LOCTEXT("NoLevels", "배틀 레벨을 찾을 수 없습니다. MWTool > Battle Levels 설정을 확인하세요."))
			.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
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
			.Text(LOCTEXT("NoDraftMonsters", "추가된 몬스터가 없습니다."))
			.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
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
				LOCTEXT("DraftMonsterRow", "{0}. {1} x{2} / 스폰 태그: {3} / 스케일: {4}"),
				FText::AsNumber(MonsterIndex + 1),
				FText::FromString(Monster.MonsterDefinition.ToSoftObjectPath().GetAssetName()),
				FText::AsNumber(Monster.Count),
				FText::FromName(Monster.SpawnTag),
				FText::AsNumber(Monster.LevelScale)
			))
			.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
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
			.Text(LOCTEXT("NoValidation", "현재 레벨 검증을 눌러 배틀 레벨을 확인하세요."))
			.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
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
	SetStatus(FText::Format(LOCTEXT("RefreshedStatus", "배틀 레벨 {0}개를 불러왔습니다."), FText::AsNumber(BattleLevels.Num())));
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::ValidateCurrentClicked()
{
	FLastFPSBattleLevelService::ValidateCurrentBattleLevel(ValidationMessages);
	RefreshValidationMessages();
	SetStatus(LOCTEXT("ValidatedStatus", "현재 레벨 검증이 완료되었습니다."));
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::AddMonsterClicked()
{
	const FSoftObjectPath MonsterPath(PendingMonsterObjectPath);
	if (!MonsterPath.IsValid())
	{
		SetStatus(LOCTEXT("MonsterMissingStatus", "먼저 몬스터 정의를 선택하세요."));
		return FReply::Handled();
	}

	FLastFPSBattleScenarioMonsterEntry MonsterEntry;
	MonsterEntry.MonsterDefinition = TSoftObjectPtr<ULastFPSCharacterDefinition>(MonsterPath);
	MonsterEntry.Count = FMath::Max(PendingMonsterCount, 1);
	MonsterEntry.SpawnTag = PendingMonsterSpawnTag.IsNone() ? FName(TEXT("EnemySpawn")) : PendingMonsterSpawnTag;
	MonsterEntry.LevelScale = FMath::Max(PendingMonsterLevelScale, 0.01f);

	DraftMonsters.Add(MonsterEntry);
	RefreshMonsterDraftList();
	SetStatus(LOCTEXT("MonsterAddedStatus", "몬스터 항목을 추가했습니다."));
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::ClearMonstersClicked()
{
	DraftMonsters.Reset();
	RefreshMonsterDraftList();
	SetStatus(LOCTEXT("MonstersClearedStatus", "몬스터 항목을 비웠습니다."));
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
		SetStatus(FText::Format(LOCTEXT("ScenarioCreatedStatus", "시나리오를 생성했습니다: {0}"), FText::FromString(SavedObjectPath)));
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
		SetStatus(FText::Format(LOCTEXT("ScenarioUpdatedStatus", "시나리오를 저장했습니다: {0}"), FText::FromString(SavedObjectPath)));
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
		SetStatus(LOCTEXT("ScenarioPlayQueuedStatus", "현재 시나리오 초안으로 PIE 실행을 예약했습니다."));
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
		SetStatus(LOCTEXT("ScenarioLoadedStatus", "선택한 시나리오를 초안에 불러왔습니다."));
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
		SetStatus(LOCTEXT("SelectedScenarioPlayQueuedStatus", "선택한 시나리오로 PIE 실행을 예약했습니다."));
		return FReply::Handled();
	}

	SetStatus(ErrorMessage);
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::OpenLevelClicked(FLastFPSBattleLevelInfo BattleLevel)
{
	if (FLastFPSBattleLevelService::OpenBattleLevel(BattleLevel))
	{
		SetStatus(FText::Format(LOCTEXT("OpenedStatus", "{0} 레벨을 열었습니다."), FText::FromString(BattleLevel.DisplayName)));
	}

	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::PlayLevelClicked(FLastFPSBattleLevelInfo BattleLevel)
{
	if (FLastFPSBattleLevelService::PlayBattleLevel(BattleLevel))
	{
		SetStatus(FText::Format(LOCTEXT("PlayStatus", "{0} 레벨로 PIE를 시작했습니다."), FText::FromString(BattleLevel.DisplayName)));
	}

	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::UseLevelForScenarioClicked(FLastFPSBattleLevelInfo BattleLevel)
{
	BattleMapObjectPath = BattleLevel.AssetPath.ToString();
	SetStatus(FText::Format(LOCTEXT("ScenarioMapSelectedStatus", "시나리오 맵을 {0}(으)로 설정했습니다."), FText::FromString(BattleLevel.DisplayName)));
	return FReply::Handled();
}

TSharedRef<SWidget> SLastFPSBattleLevelTool::BuildScenarioEditor()
{
	return LastFPSEditorWidgets::MakeSection(
		LOCTEXT("ScenarioEditorSectionTitle", "배틀 시나리오"),
		LastFPSEditorWidgets::GetToolAccentColor(),
		SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ScenarioEditorInnerTitle", "배틀 시나리오 초안"))
				.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
				.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 11))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SelectedScenarioLabel", "선택한 시나리오"))
					.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
				]
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
					.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
					.Text(LOCTEXT("LoadSelectedScenario", "선택 항목 불러오기"))
					.OnClicked(this, &SLastFPSBattleLevelTool::LoadSelectedScenarioClicked)

				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
					.Text(LOCTEXT("PlaySelectedScenario", "선택 항목 실행"))
					.OnClicked(this, &SLastFPSBattleLevelTool::PlaySelectedScenarioClicked)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DraftScenarioLabel", "시나리오 초안"))
					.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.25f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SEditableTextBox)
					.Style(&LastFPSEditorWidgets::GetToolEditableTextBoxStyle())
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
					.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
					.Text(LOCTEXT("CreateScenario", "시나리오 생성"))
					.OnClicked(this, &SLastFPSBattleLevelTool::CreateScenarioClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
					.Text(LOCTEXT("SaveScenario", "선택 항목 저장"))
					.OnClicked(this, &SLastFPSBattleLevelTool::SaveScenarioClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
					.Text(LOCTEXT("PlayScenario", "시나리오 실행"))
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
					.Style(&LastFPSEditorWidgets::GetToolEditableTextBoxStyle())
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
					.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
					.Text(LOCTEXT("AddMonster", "몬스터 추가"))
					.OnClicked(this, &SLastFPSBattleLevelTool::AddMonsterClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
					.Text(LOCTEXT("ClearMonsters", "비우기"))
					.OnClicked(this, &SLastFPSBattleLevelTool::ClearMonstersClicked)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SBox)
				.MaxDesiredHeight(100.f)
				[
					SAssignNew(MonsterDraftBox, SScrollBox)
				]
			]
		);
}

TSharedRef<SWidget> SLastFPSBattleLevelTool::BuildLevelRow(const FLastFPSBattleLevelInfo& BattleLevel)
{
	return LastFPSEditorWidgets::MakeRowBox(
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
					.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
					.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 10))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(BattleLevel.PackageName))
					.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
				.Text(LOCTEXT("OpenLevel", "열기"))
				.OnClicked(this, &SLastFPSBattleLevelTool::OpenLevelClicked, BattleLevel)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
				.Text(LOCTEXT("PlayLevel", "실행"))
				.OnClicked(this, &SLastFPSBattleLevelTool::PlayLevelClicked, BattleLevel)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
				.Text(LOCTEXT("UseLevelForScenario", "사용"))
				.OnClicked(this, &SLastFPSBattleLevelTool::UseLevelForScenarioClicked, BattleLevel)
			]
		);
}

TSharedRef<SWidget> SLastFPSBattleLevelTool::BuildValidationRow(const FLastFPSBattleLevelValidationMessage& Message) const
{
	return LastFPSEditorWidgets::MakeRowBox(
			SNew(STextBlock)
			.Text(Message.Message)
			.ColorAndOpacity(GetSeverityColor(Message.Severity))
		);
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
		? LOCTEXT("NoCurrentLevelText", "현재: 없음")
		: FText::Format(LOCTEXT("CurrentLevelText", "현재: {0}"), FText::FromString(CurrentPackageName));
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
