#include "SCharacterDataAssetTool.h"

#include "LastFPSEditorWidgets.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Brushes/SlateImageBrush.h"
#include "Character/LastFPSAbilitySet.h"
#include "Character/LastFPSAIProfile.h"
#include "Animation/LastFPSLocomotionAnimationSet.h"
#include "Animation/LastFPSLocomotionAnimationSetTools.h"
#include "Data/Tables/LastFPSCharacterMasterData.h"
#include "Data/Characters/LastFPSCharacterAcceleratorData.h"
#include "Data/Characters/LastFPSCharacterStatData.h"
#include "Data/Characters/LastFPSCharacterVisualData.h"
#include "ContentBrowserModule.h"
#include "Settings/MW_Settings.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Definitions/LastFPSHeroDefinition.h"
#include "Data/Definitions/LastFPSEnemyDefinition.h"
#include "IContentBrowserSingleton.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PropertyCustomizationHelpers.h"
#include "UObject/Package.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SCharacterDataAssetTool"

namespace
{
	FString GetCharacterToolImagePath(const TCHAR* FileName)
	{
		static const FString ResourceDirectory = []()
		{
			if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EditorUtility")))
			{
				return Plugin->GetBaseDir() / TEXT("Resources/ToolPanel");
			}

			return FPaths::ProjectDir() / TEXT("Plugins/EditorUtility/Resources/ToolPanel");
		}();

		return ResourceDirectory / FileName;
	}

	const FSlateBrush* GetCharacterToolImageTestBrush()
	{
		static const FSlateImageBrush Brush(GetCharacterToolImagePath(TEXT("TP_AssetPreview.png")), FVector2D(832.f, 260.f));
		return &Brush;
	}

	FText GetDefinitionTypeDisplayText(const FString& DefinitionType)
	{
		if (DefinitionType == TEXT("Enemy"))
		{
			return LOCTEXT("DefinitionTypeEnemy", "적");
		}

		return LOCTEXT("DefinitionTypeHero", "영웅");
	}
}

void SCharacterDataAssetTool::Construct(const FArguments& InArgs)
{
	LoadSettings();
	RefreshRowNames();

	DefinitionTypeOptions.Reset();
	DefinitionTypeOptions.Add(MakeShared<FString>(TEXT("Hero")));
	DefinitionTypeOptions.Add(MakeShared<FString>(TEXT("Enemy")));
	SelectedDefinitionType = DefinitionTypeOptions[0];

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot().FillSize(1)
		[
			LastFPSEditorWidgets::MakeToolPanel(
				LOCTEXT("CharacterToolPanelTitle", "캐릭터 도구"),
				LOCTEXT("CharacterToolPanelSubtitle", "데이터 에셋"),
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SBox)
					.HeightOverride(72.f)
					[
						SNew(SImage)
						.Image(GetCharacterToolImageTestBrush())
					]
				]

				// 섹션 1: 캐릭터 마스터 데이터 테이블
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					LastFPSEditorWidgets::MakeSection(
						LOCTEXT("MasterTableLabel", "캐릭터 마스터 데이터 테이블"),
						LastFPSEditorWidgets::GetToolAccentColor(),
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 8.f, 0.f, 6.f)
						[
							LastFPSEditorWidgets::MakeFormRow(
								LOCTEXT("MasterTableRowLabel", "데이터 테이블"),
								SNew(SObjectPropertyEntryBox)
								.AllowedClass(UDataTable::StaticClass())
								.ObjectPath(this, &SCharacterDataAssetTool::GetMasterTableObjectPath)
								.OnObjectChanged(this, &SCharacterDataAssetTool::SetMasterTable))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 6.f)
						[
							LastFPSEditorWidgets::MakeFormRow(
								LOCTEXT("RowNameLabel", "이름"),
								SAssignNew(RowNameComboBox, SComboBox<TSharedPtr<FName>>)
								.OptionsSource(&RowNames)
								.OnGenerateWidget(this, &SCharacterDataAssetTool::GenerateRowNameWidget)
								.OnSelectionChanged_Lambda([this](TSharedPtr<FName> NewSelection, ESelectInfo::Type)
								{
									SelectedRowName = NewSelection;
									SelectedRowNameString = SelectedRowName.IsValid() ? SelectedRowName->ToString() : FString();
									UpdateDefaultDefinitionTypeFromRow();
									SaveSettings();
								})
								[
									SNew(STextBlock)
									.Text(this, &SCharacterDataAssetTool::GetSelectedRowNameText)
								])
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 6.f)
						[
							LastFPSEditorWidgets::MakeFormRow(
								LOCTEXT("DefinitionTypeLabel", "정의 타입"),
								SAssignNew(DefinitionTypeComboBox, SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&DefinitionTypeOptions)
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
								{
									return SNew(STextBlock)
										.Text(InItem.IsValid() ? GetDefinitionTypeDisplayText(*InItem) : FText::GetEmpty());
								})
								.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewSelection, ESelectInfo::Type)
								{
									if (NewSelection.IsValid())
									{
										SelectedDefinitionType = NewSelection;
									}
								})
								[
									SNew(STextBlock)
									.Text_Lambda([this]()
									{
										return GetDefinitionTypeDisplayText(
											SelectedDefinitionType.IsValid() ? *SelectedDefinitionType : FString(TEXT("Hero")));
									})
								])
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							DrawFolderPickerSection(
								LOCTEXT("DefinitionOutputRootLabel", "정의 저장 경로"),
								&SCharacterDataAssetTool::DefinitionOutputRoot,
								LOCTEXT("GenerateDefinitionButton", "생성"),
								&SCharacterDataAssetTool::GenerateCharacterDefinition)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							DrawFolderPickerSection(
								LOCTEXT("StatOutputRootLabel", "스탯 저장 경로"),
								&SCharacterDataAssetTool::StatOutputRoot,
								LOCTEXT("GenerateStatButton", "스탯 생성"),
								&SCharacterDataAssetTool::GenerateCharacterStat)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							DrawFolderPickerSection(
								LOCTEXT("AbilitySetOutputRootLabel", "어빌리티 세트 저장 경로"),
								&SCharacterDataAssetTool::AbilitySetOutputRoot,
								LOCTEXT("GenerateAbilityButton", "어빌리티 세트 생성"),
								&SCharacterDataAssetTool::GenerateCharacterAbilitySet)
						]
					)
				]

				// 섹션 2: 로코모션 애니메이션 세트
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 4.f, 0.f, 8.f)
				[
					LastFPSEditorWidgets::MakeSection(
						LOCTEXT("LocomotionAnimationSetLabel", "로코모션 애니메이션 세트"),
						LastFPSEditorWidgets::GetToolAccentColor(),
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 8.f, 0.f, 6.f)
						[
							LastFPSEditorWidgets::MakeFormRow(
								LOCTEXT("LocomotionAnimationSetRowLabel", "애니메이션 세트"),
								SNew(SObjectPropertyEntryBox)
								.AllowedClass(ULastFPSLocomotionAnimationSet::StaticClass())
								.ObjectPath(this, &SCharacterDataAssetTool::GetLocomotionAnimationSetObjectPath)
								.OnObjectChanged(this, &SCharacterDataAssetTool::SetLocomotionAnimationSet))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 6.f)
						[
							LastFPSEditorWidgets::MakeFormRow(
								LOCTEXT("LocomotionAnimationNameFilterLabel", "이름 필터"),
								SNew(SEditableTextBox)
								.Style(&LastFPSEditorWidgets::GetToolEditableTextBoxStyle())
								.Text_Lambda([this]()
								{
									return FText::FromString(LocomotionAnimationNameFilter);
								})
								.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
								{
									LocomotionAnimationNameFilter = NewText.ToString();
									SaveSettings();
								}))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 6.f)
						[
							LastFPSEditorWidgets::MakeFormRow(
								LOCTEXT("LocomotionAnimationPrefixFilterLabel", "접두사 필터"),
								SNew(SEditableTextBox)
								.Style(&LastFPSEditorWidgets::GetToolEditableTextBoxStyle())
								.Text_Lambda([this]()
								{
									return FText::FromString(LocomotionAnimationPrefixFilter);
								})
								.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
								{
									LocomotionAnimationPrefixFilter = NewText.ToString();
									SaveSettings();
								}))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							DrawFolderPickerSection(
								LOCTEXT("LocomotionAnimationSourceRootLabel", "로코모션 애니메이션 소스 경로"),
								&SCharacterDataAssetTool::LocomotionAnimationSourceRoot,
								LOCTEXT("AutoFillLocomotionAnimationSetButton", "로코모션 세트 자동 채우기"),
								&SCharacterDataAssetTool::AutoFillLocomotionAnimationSet,
								false)
						]
					)
				]
			)
		]
	];

	// 처음 열릴 때도 현재 선택된 행의 캐릭터 타입에 맞춰 기본 타입을 잡아준다.
	UpdateDefaultDefinitionTypeFromRow();
}

void SCharacterDataAssetTool::LoadSettings()
{
	if (!GConfig)
	{
		return;
	}

	if (const UMW_Settings* Settings = UMW_Settings::Get())
	{
		MasterTableObjectPath = Settings->CharacterMasterTable.ToSoftObjectPath().ToString();
		DefinitionOutputRoot = Settings->CharacterDefinitionOutputRoot.IsEmpty()
			                       ? DefinitionOutputRoot
			                       : Settings->CharacterDefinitionOutputRoot;
		StatOutputRoot = Settings->CharacterStatOutputRoot.IsEmpty()
			                 ? StatOutputRoot
			                 : Settings->CharacterStatOutputRoot;
		AbilitySetOutputRoot = Settings->CharacterAbilitySetOutputRoot.IsEmpty()
			                       ? AbilitySetOutputRoot
			                       : Settings->CharacterAbilitySetOutputRoot;
		LocomotionAnimationSetObjectPath = Settings->LocomotionAnimationSet.ToSoftObjectPath().ToString();
		LocomotionAnimationSourceRoot = Settings->LocomotionAnimationSourceRoot.IsEmpty()
			                                ? LocomotionAnimationSourceRoot
			                                : Settings->LocomotionAnimationSourceRoot;
		LocomotionAnimationNameFilter = Settings->LocomotionAnimationNameFilter;
		LocomotionAnimationPrefixFilter = Settings->LocomotionAnimationPrefixFilter;
		MasterTable = Settings->CharacterMasterTable.LoadSynchronous();
		LocomotionAnimationSet = Settings->LocomotionAnimationSet.LoadSynchronous();
	}

	if (!MasterTable.IsValid() && !MasterTableObjectPath.IsEmpty())
	{
		MasterTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *MasterTableObjectPath));
	}

	if (!LocomotionAnimationSet.IsValid() && !LocomotionAnimationSetObjectPath.IsEmpty())
	{
		LocomotionAnimationSet = Cast<ULastFPSLocomotionAnimationSet>(
			StaticLoadObject(ULastFPSLocomotionAnimationSet::StaticClass(), nullptr,
			                 *LocomotionAnimationSetObjectPath));
	}
}

void SCharacterDataAssetTool::SaveSettings() const
{
	if (UMW_Settings* Settings = GetMutableDefault<UMW_Settings>())
	{
		Settings->CharacterMasterTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(MasterTableObjectPath));
		Settings->CharacterDefinitionOutputRoot = DefinitionOutputRoot;
		Settings->CharacterStatOutputRoot = StatOutputRoot;
		Settings->CharacterAbilitySetOutputRoot = AbilitySetOutputRoot;
		Settings->LocomotionAnimationSet = TSoftObjectPtr<ULastFPSLocomotionAnimationSet>(
			FSoftObjectPath(LocomotionAnimationSetObjectPath));
		Settings->LocomotionAnimationSourceRoot = LocomotionAnimationSourceRoot;
		Settings->LocomotionAnimationNameFilter = LocomotionAnimationNameFilter;
		Settings->LocomotionAnimationPrefixFilter = LocomotionAnimationPrefixFilter;
		Settings->SaveConfig();
	}
}

void SCharacterDataAssetTool::SetMasterTable(const FAssetData& AssetData)
{
	MasterTable = Cast<UDataTable>(AssetData.GetAsset());
	MasterTableObjectPath = AssetData.IsValid() ? AssetData.GetSoftObjectPath().ToString() : FString();
	RefreshRowNames();
	SaveSettings();
}

FString SCharacterDataAssetTool::GetMasterTableObjectPath() const
{
	return MasterTableObjectPath;
}

void SCharacterDataAssetTool::SetLocomotionAnimationSet(const FAssetData& AssetData)
{
	LocomotionAnimationSet = Cast<ULastFPSLocomotionAnimationSet>(AssetData.GetAsset());
	LocomotionAnimationSetObjectPath = AssetData.IsValid() ? AssetData.GetSoftObjectPath().ToString() : FString();
	SaveSettings();
}

FString SCharacterDataAssetTool::GetLocomotionAnimationSetObjectPath() const
{
	return LocomotionAnimationSetObjectPath;
}

void SCharacterDataAssetTool::RefreshRowNames()
{
	RowNames.Reset();
	SelectedRowName.Reset();

	if (UDataTable* Table = MasterTable.Get())
	{
		for (const FName& RowName : Table->GetRowNames())
		{
			RowNames.Add(MakeShared<FName>(RowName));
		}

		if (RowNames.Num() > 0)
		{
			SelectedRowName = RowNames[0];
			for (const TSharedPtr<FName>& RowName : RowNames)
			{
				if (RowName.IsValid() && RowName->ToString() == SelectedRowNameString)
				{
					SelectedRowName = RowName;
					break;
				}
			}
			SelectedRowNameString = SelectedRowName.IsValid() ? SelectedRowName->ToString() : FString();
		}
	}
	else
	{
		SelectedRowNameString.Empty();
	}

	if (RowNameComboBox.IsValid())
	{
		RowNameComboBox->RefreshOptions();
		RowNameComboBox->SetSelectedItem(SelectedRowName);
	}
}

TSharedRef<SWidget> SCharacterDataAssetTool::GenerateRowNameWidget(TSharedPtr<FName> RowName) const
{
	return SNew(STextBlock)
		.Text(RowName.IsValid() ? FText::FromName(*RowName) : FText::GetEmpty());
}

FText SCharacterDataAssetTool::GetSelectedRowNameText() const
{
	return SelectedRowName.IsValid()
		       ? FText::FromName(*SelectedRowName)
		       : LOCTEXT("NoRowSelected", "행을 선택하세요");
}

FReply SCharacterDataAssetTool::OpenFolderPicker(FString SCharacterDataAssetTool::* OutputRootMember)
{
	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = (this->*OutputRootMember);
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateLambda(
		[this, OutputRootMember](const FString& SelectedPath)
		{
			if (!SelectedPath.IsEmpty())
			{
				(this->*OutputRootMember) = SelectedPath;
				SaveSettings();
			}
		});

	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(LOCTEXT("OutputRootPickerTitle", "캐릭터 에셋 폴더 선택"))
		.ClientSize(FVector2D(420.f, 520.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TWeakPtr<SWindow> WeakPickerWindow = PickerWindow;
	PickerWindow->SetContent(
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(8.f)
		[
			ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f)
		.HAlign(HAlign_Right)
		[
			SNew(SButton)
			.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
			.Text(LOCTEXT("UseFolderButton", "선택한 폴더 사용"))
			.OnClicked_Lambda([WeakPickerWindow]()
			{
				if (TSharedPtr<SWindow> PickerWindowPtr = WeakPickerWindow.Pin())
				{
					PickerWindowPtr->RequestDestroyWindow();
				}
				return FReply::Handled();
			})
		]);

	FSlateApplication::Get().AddWindow(PickerWindow);
	return FReply::Handled();
}

FReply SCharacterDataAssetTool::GenerateCharacterDefinition()
{
	UDataTable* Table = MasterTable.Get();
	if (!Table || !SelectedRowName.IsValid())
	{
		return FReply::Handled();
	}

	static const FString Context(TEXT("CharacterDataAssetTool"));
	const FLastFPSCharacterMasterData* Row =
		Table->FindRow<FLastFPSCharacterMasterData>(*SelectedRowName, Context, true);
	if (!Row || !Row->bGenerateDefinition)
	{
		return FReply::Handled();
	}

	const FString CharacterIdString = Row->CharacterId.IsNone()
		                                  ? SelectedRowName->ToString()
		                                  : Row->CharacterId.ToString();

	const FString AssetName = Row->DefinitionAssetName.IsEmpty()
		                          ? FString::Printf(TEXT("DA_Char_%s"), *CharacterIdString)
		                          : Row->DefinitionAssetName;

	ULastFPSCharacterDefinition* Definition = Row->TargetDefinition.LoadSynchronous();
	if (!Definition)
	{
		const FString PackageName = DefinitionOutputRoot / AssetName;
		if (!FPackageName::IsValidLongPackageName(PackageName))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid character definition package path: %s"), *PackageName);
			return FReply::Handled();
		}

		if (UObject* ExistingAsset = StaticLoadObject(
			ULastFPSCharacterDefinition::StaticClass(),
			nullptr,
			*(PackageName + TEXT(".") + AssetName)))
		{
			Definition = Cast<ULastFPSCharacterDefinition>(ExistingAsset);
		}
		else
		{
			UPackage* Package = CreatePackage(*PackageName);
			// 콤보에서 고른 타입으로 서브클래스 인스턴스를 생성한다.
			Definition = NewObject<ULastFPSCharacterDefinition>(
				Package,
				ResolveDefinitionClass(),
				*AssetName,
				RF_Public | RF_Standalone | RF_Transactional);

			FAssetRegistryModule::AssetCreated(Definition);
		}
	}

	if (Definition)
	{
		ApplyRowToDefinition(Definition, *Row, *SelectedRowName);
		Definition->MarkPackageDirty();

		TArray<UObject*> ObjectsToSync;
		ObjectsToSync.Add(Definition);
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}

	return FReply::Handled();
}

bool SCharacterDataAssetTool::CanGenerate() const
{
	return MasterTable.IsValid() && SelectedRowName.IsValid();
}

TSharedRef<SWidget> SCharacterDataAssetTool::DrawFolderPickerSection(
	const FText& LabelText,
	FString SCharacterDataAssetTool::* OutputRootMember,
	const FText& GenerateButtonText,
	FReply (SCharacterDataAssetTool::*OnGenerateClicked)(),
	bool bRequiresCharacterRow)
{
	return LastFPSEditorWidgets::MakeFormRow(
		LabelText,
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			SNew(SEditableTextBox)
			.Style(&LastFPSEditorWidgets::GetToolEditableTextBoxStyle())
			.IsReadOnly(true)
			.Text_Lambda([this, OutputRootMember]()
			{
				return FText::FromString((this->*OutputRootMember));
			})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
			.Text(LOCTEXT("PickFolderButton", "폴더 선택"))
			.OnClicked_Lambda([this, OutputRootMember]()
			{
				return OpenFolderPicker(OutputRootMember);
			})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(&LastFPSEditorWidgets::GetToolButtonStyle())
			.ContentPadding(FMargin(12.f, 4.f))
			.Text(GenerateButtonText)
			.IsEnabled_Lambda([this, OutputRootMember, bRequiresCharacterRow]()
			{
				return (!bRequiresCharacterRow || CanGenerate()) && !(this->*OutputRootMember).IsEmpty();
			})
			.OnClicked(this, OnGenerateClicked)
		]);
}

FReply SCharacterDataAssetTool::GenerateCharacterStat()
{
	UDataTable* Table = MasterTable.Get();
	if (!Table || !SelectedRowName.IsValid())
	{
		return FReply::Handled();
	}

	static const FString Context(TEXT("CharacterDataAssetTool"));
	const FLastFPSCharacterMasterData* Row =
		Table->FindRow<FLastFPSCharacterMasterData>(*SelectedRowName, Context, true);
	if (!Row)
	{
		return FReply::Handled();
	}

	ULastFPSCharacterStatData* StatData = Row->StatData.LoadSynchronous();
	if (!StatData)
	{
		const FString CharacterIdString = Row->CharacterId.IsNone()
			                                  ? SelectedRowName->ToString()
			                                  : Row->CharacterId.ToString();

		const FString AssetName = FString::Printf(TEXT("DA_Char_%s_Stat"), *CharacterIdString);
		const FString PackageName = StatOutputRoot / AssetName;

		if (!FPackageName::IsValidLongPackageName(PackageName))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid character definition package path: %s"),
			       *PackageName);
			return FReply::Handled();
		}

		if (UObject* ExistingAsset = StaticLoadObject(ULastFPSCharacterStatData::StaticClass(),
		                                              nullptr, *(PackageName + TEXT(".") + AssetName)))
		{
			StatData = Cast<ULastFPSCharacterStatData>(ExistingAsset);
		}
		else
		{
			UPackage* Package = CreatePackage(*PackageName);
			StatData = NewObject<ULastFPSCharacterStatData>(Package, ULastFPSCharacterStatData::StaticClass(),
			                                                *AssetName, RF_Public | RF_Standalone | RF_Transactional);

			FAssetRegistryModule::AssetCreated(StatData);
		}
	}

	if (StatData)
	{
		StatData->MarkPackageDirty();

		TArray<UObject*> ObjectsToSync;
		ObjectsToSync.Add(StatData);
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}

	return FReply::Handled();
}

FReply SCharacterDataAssetTool::GenerateCharacterAbilitySet()
{
	UDataTable* Table = MasterTable.Get();
	if (!Table || !SelectedRowName.IsValid())
	{
		return FReply::Handled();
	}

	static const FString Context(TEXT("CharacterDataAssetTool"));
	const FLastFPSCharacterMasterData* Row =
		Table->FindRow<FLastFPSCharacterMasterData>(*SelectedRowName, Context, true);
	if (!Row)
	{
		return FReply::Handled();
	}

	ULastFPSAbilitySet* AbilitySet = Row->AbilitySet.LoadSynchronous();
	if (!AbilitySet)
	{
		const FString CharacterIdString = Row->CharacterId.IsNone()
			                                  ? SelectedRowName->ToString()
			                                  : Row->CharacterId.ToString();

		const FString AssetName = FString::Printf(TEXT("DA_Char_%s_AbilitySet"), *CharacterIdString);
		const FString PackageName = AbilitySetOutputRoot / AssetName;

		if (!FPackageName::IsValidLongPackageName(PackageName))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid character ability set package path: %s"), *PackageName);
			return FReply::Handled();
		}

		if (UObject* ExistingAsset = StaticLoadObject(
			ULastFPSAbilitySet::StaticClass(),
			nullptr,
			*(PackageName + TEXT(".") + AssetName)))
		{
			AbilitySet = Cast<ULastFPSAbilitySet>(ExistingAsset);
		}
		else
		{
			UPackage* Package = CreatePackage(*PackageName);
			AbilitySet = NewObject<ULastFPSAbilitySet>(
				Package,
				ULastFPSAbilitySet::StaticClass(),
				*AssetName,
				RF_Public | RF_Standalone | RF_Transactional);

			FAssetRegistryModule::AssetCreated(AbilitySet);
		}
	}

	if (AbilitySet)
	{
		AbilitySet->MarkPackageDirty();

		TArray<UObject*> ObjectsToSync;
		ObjectsToSync.Add(AbilitySet);
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}

	return FReply::Handled();
}

FReply SCharacterDataAssetTool::AutoFillLocomotionAnimationSet()
{
	ULastFPSLocomotionAnimationSet* AnimationSet = LocomotionAnimationSet.Get();
	if (!AnimationSet && !LocomotionAnimationSetObjectPath.IsEmpty())
	{
		AnimationSet = Cast<ULastFPSLocomotionAnimationSet>(
			StaticLoadObject(ULastFPSLocomotionAnimationSet::StaticClass(), nullptr,
			                 *LocomotionAnimationSetObjectPath));
		LocomotionAnimationSet = AnimationSet;
	}

	if (!AnimationSet || LocomotionAnimationSourceRoot.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Locomotion animation auto fill failed: AnimationSet or source root is empty."));
		return FReply::Handled();
	}

	const int32 AssignedCount = ULastFPSLocomotionAnimationSetTools::AutoFillLocomotionAnimationSetWithFilters(
		AnimationSet,
		LocomotionAnimationSourceRoot,
		LocomotionAnimationNameFilter,
		LocomotionAnimationPrefixFilter,
		true,
		true);

	if (AssignedCount > 0)
	{
		TArray<UObject*> ObjectsToSync;
		ObjectsToSync.Add(AnimationSet);
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}

	SaveSettings();
	return FReply::Handled();
}

UObject* SCharacterDataAssetTool::LoadSoftObject(const FSoftObjectPath& Path) const
{
	return Path.IsValid() ? Path.TryLoad() : nullptr;
}

void SCharacterDataAssetTool::ApplyRowToDefinition(
	ULastFPSCharacterDefinition* Definition,
	const FLastFPSCharacterMasterData& Row,
	const FName RowName) const
{
	Definition->Modify();
	Definition->CharacterId = Row.CharacterId.IsNone() ? RowName : Row.CharacterId;
	Definition->CharacterType = Row.CharacterType;
	Definition->ClassificationTags = Row.ClassificationTags;
	Definition->DisplayName = FText::FromString(Row.DisplayName);
	Definition->Icon = Cast<UTexture2D>(Row.Icon.LoadSynchronous());
	Definition->PawnClass = Row.PawnClass.LoadSynchronous();
	Definition->StatData = Row.StatData.LoadSynchronous();
	Definition->VisualData = Row.VisualData.LoadSynchronous();
	Definition->AcceleratorData = Row.AcceleratorData.LoadSynchronous();
	Definition->AbilitySet = Row.AbilitySet.LoadSynchronous();

	// 서브클래스 전용 필드는 실제 타입에 맞춰 채운다.
	if (ULastFPSHeroDefinition* HeroDefinition = Cast<ULastFPSHeroDefinition>(Definition))
	{
		HeroDefinition->Role = FText::FromString(Row.Role);
		HeroDefinition->Description = FText::FromString(Row.Description);
	}
	else if (ULastFPSEnemyDefinition* EnemyDefinition = Cast<ULastFPSEnemyDefinition>(Definition))
	{
		EnemyDefinition->AIProfile = Row.AIProfile.LoadSynchronous();
	}
}

UClass* SCharacterDataAssetTool::ResolveDefinitionClass() const
{
	if (SelectedDefinitionType.IsValid() && *SelectedDefinitionType == TEXT("Enemy"))
	{
		return ULastFPSEnemyDefinition::StaticClass();
	}
	return ULastFPSHeroDefinition::StaticClass();
}

void SCharacterDataAssetTool::UpdateDefaultDefinitionTypeFromRow()
{
	UDataTable* Table = MasterTable.Get();
	if (!Table || !SelectedRowName.IsValid())
	{
		return;
	}

	static const FString Context(TEXT("CharacterDataAssetTool"));
	const FLastFPSCharacterMasterData* Row =
		Table->FindRow<FLastFPSCharacterMasterData>(*SelectedRowName, Context, true);
	if (!Row)
	{
		return;
	}

	const FString DesiredType =
		(Row->CharacterType == ELastFPSCharacterType::Enemy) ? TEXT("Enemy") : TEXT("Hero");

	for (const TSharedPtr<FString>& Option : DefinitionTypeOptions)
	{
		if (Option.IsValid() && *Option == DesiredType)
		{
			SelectedDefinitionType = Option;
			break;
		}
	}

	if (DefinitionTypeComboBox.IsValid())
	{
		DefinitionTypeComboBox->SetSelectedItem(SelectedDefinitionType);
	}
}

#undef LOCTEXT_NAMESPACE
