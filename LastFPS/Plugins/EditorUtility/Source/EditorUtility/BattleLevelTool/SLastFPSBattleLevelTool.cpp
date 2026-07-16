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
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SLastFPSBattleLevelTool"

namespace
{
	// 입력 필드 위에 작은 캡션을 얹어 무엇을 입력하는지 명확히 한다.
	// 폼 컨트롤이 한 줄에 몰려 있을 때 각 항목의 의미를 잃지 않도록 하기 위한 용도.
	TSharedRef<SWidget> MakeLabeledField(const FText& Label, const TSharedRef<SWidget>& Field)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(STextBlock)
				.Text(Label)
				.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
				.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Regular")), 8))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				Field
			];
	}

	// 사용 설명서 팝업의 한 항목. 소제목(굵게)과 본문(자동 줄바꿈)을 세로로 묶는다.
	TSharedRef<SWidget> MakeHelpBlock(const FText& Heading, const FText& Body)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 3.f)
			[
				SNew(STextBlock)
				.Text(Heading)
				.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
				.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 11))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 12.f)
			[
				SNew(STextBlock)
				.Text(Body)
				.AutoWrapText(true)
				.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
			];
	}

	// 워크플로우 단계 헤딩. 번호 배지와 제목으로 순서를 드러낸다.
	TSharedRef<SWidget> MakeStepHeading(const FText& Number, const FText& Title)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock)
				.Text(Number)
				.ColorAndOpacity(FSlateColor(LastFPSEditorWidgets::GetToolAccentColor()))
				.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 11))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Title)
				.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
				.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 11))
			];
	}
}

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
					]

					// 우측 끝의 도움말(?) 버튼. 클릭 시 사용 설명서 팝업을 연다.
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(6.f, 0.f, 0.f, 0.f)
					[
						SAssignNew(HelpAnchor, SMenuAnchor)
						.Placement(MenuPlacement_BelowRightAnchor)
						.OnGetMenuContent(this, &SLastFPSBattleLevelTool::BuildHelpMenuContent)
						[
							SNew(SButton)
							.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
							.ToolTipText(LOCTEXT("HelpButtonTooltip", "사용 설명서 열기"))
							.OnClicked(this, &SLastFPSBattleLevelTool::ToggleHelpClicked)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("HelpButtonLabel", "?"))
								.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
								.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 11))
							]
						]
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
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			LastFPSEditorWidgets::MakeRowBox(
				SNew(SHorizontalBox)

				// 순번
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::AsNumber(MonsterIndex + 1))
					.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
				]

				// 캐릭터 이름과 수량
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::Format(
						LOCTEXT("DraftMonsterName", "{0}  x{1}"),
						FText::FromString(Monster.MonsterDefinition.ToSoftObjectPath().GetAssetName()),
						FText::AsNumber(Monster.Count)))
					.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
					.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 10))
				]

				// 스폰 태그와 스케일 메타
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::Format(
						LOCTEXT("DraftMonsterMeta", "{0} · 스케일 {1} · 배치 {2} · {3}cm"),
						FText::FromName(Monster.SpawnTag),
						FText::AsNumber(Monster.LevelScale),
						Monster.Formation == ELastFPSBattleFormation::Grid
							? FText::Format(LOCTEXT("DraftMonsterGridFmt", "격자({0}열)"), FText::AsNumber(FMath::Max(Monster.GridColumns, 1)))
							: GetFormationLabel(Monster.Formation),
						FText::AsNumber(Monster.FormationSpacing)))
					.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
				]

				// 행별 삭제
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
					.Text(LOCTEXT("RemoveMonster", "삭제"))
					.OnClicked(this, &SLastFPSBattleLevelTool::RemoveMonsterClicked, MonsterIndex)
				])
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

FReply SLastFPSBattleLevelTool::ToggleHelpClicked()
{
	if (HelpAnchor)
	{
		HelpAnchor->SetIsOpen(!HelpAnchor->IsOpen());
	}

	return FReply::Handled();
}

TSharedRef<SWidget> SLastFPSBattleLevelTool::BuildHelpMenuContent()
{
	const TSharedRef<SScrollBox> HelpBody = SNew(SScrollBox);

	HelpBody->AddSlot()
	.Padding(0.f, 0.f, 0.f, 4.f)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("HelpTitle", "배틀 레벨 도구 사용 설명서"))
		.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
		.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 13))
	];

	HelpBody->AddSlot()
	.Padding(0.f, 0.f, 0.f, 10.f)
	[
		LastFPSEditorWidgets::MakeColorLine(LastFPSEditorWidgets::GetToolAccentColor())
	];

	HelpBody->AddSlot()
	[
		MakeHelpBlock(
			LOCTEXT("HelpOverviewHeading", "개요"),
			LOCTEXT("HelpOverviewBody",
				"배틀 레벨과 전투 시나리오를 한 곳에서 만들고 편집하고 PIE로 바로 시험하는 도구입니다. "
				"시나리오는 대상 맵, 플레이어 캐릭터, 스폰할 몬스터 구성을 담아 BS_ 데이터 에셋으로 저장됩니다."))
	];

	HelpBody->AddSlot()
	[
		MakeHelpBlock(
			LOCTEXT("HelpCurrentHeading", "현재 레벨"),
			LOCTEXT("HelpCurrentBody",
				"· 새로고침: 프로젝트에서 배틀 레벨 목록을 다시 수집합니다.\n"
				"· 현재 레벨 검증: 지금 열려 있는 레벨을 검사해 스폰 지점 태그 등 실행 조건이 갖춰졌는지 아래 검증 영역에 보여줍니다."))
	];

	HelpBody->AddSlot()
	[
		MakeHelpBlock(
			LOCTEXT("HelpStep1Heading", "① 저장된 시나리오 불러오기"),
			LOCTEXT("HelpStep1Body",
				"· 불러오기: 선택한 시나리오(BS_) 에셋의 내용을 아래 초안 편집 영역으로 채웁니다.\n"
				"· 실행: 선택한 시나리오를 초안으로 가져오지 않고 그대로 PIE로 실행합니다."))
	];

	HelpBody->AddSlot()
	[
		MakeHelpBlock(
			LOCTEXT("HelpStep2Heading", "② 시나리오 초안 편집"),
			LOCTEXT("HelpStep2Body",
				"· 시나리오 이름: 새로 생성할 때 사용할 에셋 이름입니다.\n"
				"· 대상 맵 / 플레이어 캐릭터: 전투가 벌어질 레벨과 플레이할 캐릭터 정의.\n"
				"· 몬스터 추가: 캐릭터·수량(1 이상)·스폰 태그·스케일(0.01 이상)을 정하고 추가를 누르면 목록에 들어갑니다.\n"
				"· 스폰 태그는 레벨에 배치된 스폰 지점의 태그와 일치해야 실제로 스폰됩니다(기본 EnemySpawn).\n"
				"· 스폰 목록: 각 행의 삭제로 개별 제거, 비우기로 전체 초기화."))
	];

	HelpBody->AddSlot()
	[
		MakeHelpBlock(
			LOCTEXT("HelpActionsHeading", "저장 · 실행"),
			LOCTEXT("HelpActionsBody",
				"· 시나리오 생성: 현재 초안을 새 시나리오 에셋으로 만듭니다.\n"
				"· 저장: 불러온(또는 방금 생성한) 시나리오 에셋에 현재 초안을 덮어써 갱신합니다.\n"
				"· 초안으로 실행: 에셋으로 저장하지 않고 현재 초안 그대로 PIE를 실행합니다. 빠른 반복 테스트용입니다."))
	];

	HelpBody->AddSlot()
	[
		MakeHelpBlock(
			LOCTEXT("HelpLevelsHeading", "배틀 레벨 목록"),
			LOCTEXT("HelpLevelsBody",
				"· 열기: 해당 레벨을 에디터에서 엽니다.\n"
				"· 실행: 해당 레벨로 PIE를 시작합니다.\n"
				"· 사용: 해당 레벨을 초안의 대상 맵으로 지정합니다."))
	];

	HelpBody->AddSlot()
	[
		MakeHelpBlock(
			LOCTEXT("HelpValidationHeading", "검증 메시지 색"),
			LOCTEXT("HelpValidationBody",
				"· 초록: 정보/정상  · 노랑: 경고(실행은 되지만 확인 필요)  · 빨강: 오류(수정 필요).\n"
				"실행 전 현재 레벨 검증으로 스폰 태그 구성을 확인하면 스폰 누락을 예방할 수 있습니다."))
	];

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		.Padding(FMargin(14.f, 12.f))
		[
			SNew(SBox)
			.WidthOverride(500.f)
			.MaxDesiredHeight(560.f)
			[
				HelpBody
			]
		];
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
	MonsterEntry.Formation = PendingMonsterFormation;
	MonsterEntry.FormationSpacing = FMath::Max(PendingMonsterFormationSpacing, 0.f);
	MonsterEntry.GridColumns = FMath::Max(PendingMonsterGridColumns, 1);

	DraftMonsters.Add(MonsterEntry);
	RefreshMonsterDraftList();
	SetStatus(LOCTEXT("MonsterAddedStatus", "몬스터 항목을 추가했습니다."));
	return FReply::Handled();
}

FReply SLastFPSBattleLevelTool::RemoveMonsterClicked(int32 MonsterIndex)
{
	if (!DraftMonsters.IsValidIndex(MonsterIndex))
	{
		return FReply::Handled();
	}

	DraftMonsters.RemoveAt(MonsterIndex);
	RefreshMonsterDraftList();
	SetStatus(LOCTEXT("MonsterRemovedStatus", "몬스터 항목을 삭제했습니다."));
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

		// 1단계: 저장된 시나리오를 초안으로 불러오거나 바로 실행한다.
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeStepHeading(
				LOCTEXT("StepLoadNumber", "1"),
				LOCTEXT("StepLoadTitle", "저장된 시나리오 불러오기"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
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
				.Text(LOCTEXT("LoadSelectedScenario", "불러오기"))
				.OnClicked(this, &SLastFPSBattleLevelTool::LoadSelectedScenarioClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
				.Text(LOCTEXT("PlaySelectedScenario", "실행"))
				.OnClicked(this, &SLastFPSBattleLevelTool::PlaySelectedScenarioClicked)
			]
		]

		// 2단계: 초안의 메타데이터와 몬스터 구성을 편집한다.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 12.f, 0.f, 0.f)
		[
			MakeStepHeading(
				LOCTEXT("StepEditNumber", "2"),
				LOCTEXT("StepEditTitle", "시나리오 초안 편집"))
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
				MakeLabeledField(
					LOCTEXT("ScenarioNameFieldLabel", "시나리오 이름"),
					SNew(SEditableTextBox)
					.Style(&LastFPSEditorWidgets::GetToolEditableTextBoxStyle())
					.Text(this, &SLastFPSBattleLevelTool::GetScenarioNameText)
					.OnTextCommitted(this, &SLastFPSBattleLevelTool::SetScenarioName))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.33f)
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				MakeLabeledField(
					LOCTEXT("ScenarioMapFieldLabel", "대상 맵"),
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UWorld::StaticClass())
					.ObjectPath(this, &SLastFPSBattleLevelTool::GetBattleMapObjectPath)
					.OnObjectChanged(this, &SLastFPSBattleLevelTool::SetBattleMapObject))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.33f)
			[
				MakeLabeledField(
					LOCTEXT("ScenarioPlayerFieldLabel", "플레이어 캐릭터"),
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(ULastFPSCharacterDefinition::StaticClass())
					.ObjectPath(this, &SLastFPSBattleLevelTool::GetPlayerCharacterObjectPath)
					.OnObjectChanged(this, &SLastFPSBattleLevelTool::SetPlayerCharacterObject))
			]
		]

		// 몬스터 추가 폼: 각 필드가 무엇인지 라벨로 구분한다.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 8.f, 0.f, 0.f)
		[
			LastFPSEditorWidgets::MakeRowBox(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					MakeLabeledField(
						LOCTEXT("MonsterCharacterFieldLabel", "캐릭터"),
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(ULastFPSCharacterDefinition::StaticClass())
						.ObjectPath(this, &SLastFPSBattleLevelTool::GetPendingMonsterObjectPath)
						.OnObjectChanged(this, &SLastFPSBattleLevelTool::SetPendingMonsterObject))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.14f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					MakeLabeledField(
						LOCTEXT("MonsterCountFieldLabel", "수량"),
						SNew(SSpinBox<int32>)
						.MinValue(1)
						.MaxValue(999)
						.Value_Lambda([this]() { return PendingMonsterCount; })
						.OnValueChanged_Lambda([this](int32 NewValue) { PendingMonsterCount = FMath::Max(NewValue, 1); }))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.26f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					MakeLabeledField(
						LOCTEXT("MonsterSpawnTagFieldLabel", "스폰 태그"),
						SNew(SEditableTextBox)
						.Style(&LastFPSEditorWidgets::GetToolEditableTextBoxStyle())
						.Text(this, &SLastFPSBattleLevelTool::GetPendingMonsterSpawnTagText)
						.OnTextCommitted(this, &SLastFPSBattleLevelTool::SetPendingMonsterSpawnTag))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.14f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					MakeLabeledField(
						LOCTEXT("MonsterScaleFieldLabel", "스케일"),
						SNew(SSpinBox<float>)
						.MinValue(0.01f)
						.MaxValue(100.f)
						.Value_Lambda([this]() { return PendingMonsterLevelScale; })
						.OnValueChanged_Lambda([this](float NewValue) { PendingMonsterLevelScale = FMath::Max(NewValue, 0.01f); }))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Bottom)
				[
					SNew(SButton)
					.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
					.Text(LOCTEXT("AddMonster", "추가"))
					.OnClicked(this, &SLastFPSBattleLevelTool::AddMonsterClicked)
				])
		]

		// 배치 방식 선택: 가로/세로/격자 + 간격/열. 추가 시 몬스터 항목에 반영된다.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 6.f, 0.f, 0.f)
		[
			BuildFormationSelector()
		]

		// 스폰 목록 헤더: 현재 개수와 전체 비우기 액션.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 8.f, 0.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SLastFPSBattleLevelTool::GetDraftMonsterCountText)
				.ColorAndOpacity(LastFPSEditorWidgets::GetToolMutedTextColor())
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
		[
			SNew(SBox)
			.MaxDesiredHeight(110.f)
			[
				SAssignNew(MonsterDraftBox, SScrollBox)
			]
		]

		// 초안 저장/실행 액션. 실행은 강조 버튼으로 위계를 둔다.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 10.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("CreateScenario", "시나리오 생성"))
				.OnClicked(this, &SLastFPSBattleLevelTool::CreateScenarioClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("SaveScenario", "저장"))
				.OnClicked(this, &SLastFPSBattleLevelTool::SaveScenarioClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SButton)
				.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("PlayScenario", "초안으로 실행"))
				.OnClicked(this, &SLastFPSBattleLevelTool::PlayScenarioClicked)
			]
		]
	);
}

TSharedRef<SWidget> SLastFPSBattleLevelTool::BuildFormationSelector()
{
	// 배치 방식 토글 버튼. 활성 항목은 강조색 라벨로 표시된다.
	auto MakeFormationButton = [this](ELastFPSBattleFormation Formation)
	{
		return SNew(SButton)
			.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
			.OnClicked(this, &SLastFPSBattleLevelTool::SetPendingFormationClicked, Formation)
			[
				SNew(STextBlock)
				.Text(GetFormationLabel(Formation))
				.ColorAndOpacity_Lambda([this, Formation]()
				{
					return FSlateColor(PendingMonsterFormation == Formation
						? LastFPSEditorWidgets::GetToolAccentColor()
						: LastFPSEditorWidgets::GetToolMutedTextColor());
				})
			];
	};

	return LastFPSEditorWidgets::MakeRowBox(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 10.f, 0.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FormationLabel", "배치"))
			.ColorAndOpacity(LastFPSEditorWidgets::GetToolTextColor())
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
		[ MakeFormationButton(ELastFPSBattleFormation::Horizontal) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
		[ MakeFormationButton(ELastFPSBattleFormation::Vertical) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
		[ MakeFormationButton(ELastFPSBattleFormation::Grid) ]

		+ SHorizontalBox::Slot()
		.FillWidth(0.42f)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			MakeLabeledField(
				LOCTEXT("FormationSpacingLabel", "간격(cm)"),
				SNew(SSpinBox<float>)
				.MinValue(0.f)
				.MaxValue(2000.f)
				.Value_Lambda([this]() { return PendingMonsterFormationSpacing; })
				.OnValueChanged_Lambda([this](float NewValue) { PendingMonsterFormationSpacing = FMath::Max(NewValue, 0.f); }))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.30f)
		[
			MakeLabeledField(
				LOCTEXT("FormationColumnsLabel", "열(격자)"),
				SNew(SSpinBox<int32>)
				.MinValue(1)
				.MaxValue(50)
				// 격자일 때만 의미가 있으므로 다른 배치에서는 비활성화한다.
				.IsEnabled_Lambda([this]() { return PendingMonsterFormation == ELastFPSBattleFormation::Grid; })
				.Value_Lambda([this]() { return PendingMonsterGridColumns; })
				.OnValueChanged_Lambda([this](int32 NewValue) { PendingMonsterGridColumns = FMath::Max(NewValue, 1); }))
		]
	);
}

FReply SLastFPSBattleLevelTool::SetPendingFormationClicked(ELastFPSBattleFormation Formation)
{
	PendingMonsterFormation = Formation;
	return FReply::Handled();
}

FText SLastFPSBattleLevelTool::GetFormationLabel(ELastFPSBattleFormation Formation) const
{
	switch (Formation)
	{
	case ELastFPSBattleFormation::Vertical: return LOCTEXT("FormationVertical", "세로");
	case ELastFPSBattleFormation::Grid:     return LOCTEXT("FormationGrid", "격자");
	case ELastFPSBattleFormation::Horizontal:
	default:                                 return LOCTEXT("FormationHorizontal", "가로");
	}
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

FText SLastFPSBattleLevelTool::GetDraftMonsterCountText() const
{
	return FText::Format(LOCTEXT("DraftMonsterCount", "스폰 목록 · {0}"), FText::AsNumber(DraftMonsters.Num()));
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
