#include "SCharacterDataAssetTool.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/LastFPSAbilitySet.h"
#include "Character/LastFPSAIProfile.h"
#include "Animation/LastFPSLocomotionAnimationSet.h"
#include "Animation/LastFPSLocomotionAnimationSetTools.h"
#include "Data/Tables/LastFPSCharacterMasterData.h"
#include "Data/Characters/LastFPSCharacterStatData.h"
#include "Data/Characters/LastFPSCharacterVisualData.h"
#include "ContentBrowserModule.h"
#include "Settings/EUW_Settings.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"
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
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SCharacterDataAssetTool"

void SCharacterDataAssetTool::Construct(const FArguments& InArgs)
{
	LoadSettings();
	RefreshRowNames();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MasterTableLabel", "Character Master DataTable"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 12.f)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UDataTable::StaticClass())
				.ObjectPath(this, &SCharacterDataAssetTool::GetMasterTableObjectPath)
				.OnObjectChanged(this, &SCharacterDataAssetTool::SetMasterTable)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RowNameLabel", "Name"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 12.f)
			[
				SAssignNew(RowNameComboBox, SComboBox<TSharedPtr<FName>>)
				.OptionsSource(&RowNames)
				.OnGenerateWidget(this, &SCharacterDataAssetTool::GenerateRowNameWidget)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FName> NewSelection, ESelectInfo::Type)
				{
					SelectedRowName = NewSelection;
					SelectedRowNameString = SelectedRowName.IsValid() ? SelectedRowName->ToString() : FString();
					SaveSettings();
				})
				[
					SNew(STextBlock)
					.Text(this, &SCharacterDataAssetTool::GetSelectedRowNameText)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				DrawFolderPickerSection(
					LOCTEXT("DefinitionOutputRootLabel", "Definition Output Root"),
					&SCharacterDataAssetTool::DefinitionOutputRoot,
					LOCTEXT("GenerateDefinitionButton", "Generate"),
					&SCharacterDataAssetTool::GenerateCharacterDefinition)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				DrawFolderPickerSection(
					LOCTEXT("StatOutputRootLabel", "Stat Output Root"),
					&SCharacterDataAssetTool::StatOutputRoot,
					LOCTEXT("GenerateStatButton", "Generate Stat"),
					&SCharacterDataAssetTool::GenerateCharacterStat)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				DrawFolderPickerSection(
					LOCTEXT("AbilitySetOutputRootLabel", "AbilitySet Output Root"),
					&SCharacterDataAssetTool::AbilitySetOutputRoot,
					LOCTEXT("GenerateAbilityButton", "Generate AbilitySet"),
					&SCharacterDataAssetTool::GenerateCharacterAbilitySet)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LocomotionAnimationSetLabel", "Locomotion Animation Set"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 12.f)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(ULastFPSLocomotionAnimationSet::StaticClass())
				.ObjectPath(this, &SCharacterDataAssetTool::GetLocomotionAnimationSetObjectPath)
				.OnObjectChanged(this, &SCharacterDataAssetTool::SetLocomotionAnimationSet)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LocomotionAnimationNameFilterLabel", "Locomotion Animation Name Filter"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 16.f)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([this]()
				{
					return FText::FromString(LocomotionAnimationNameFilter);
				})
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
				{
					LocomotionAnimationNameFilter = NewText.ToString();
					SaveSettings();
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LocomotionAnimationPrefixFilterLabel", "Locomotion Animation Prefix Filter"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 16.f)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([this]()
				{
					return FText::FromString(LocomotionAnimationPrefixFilter);
				})
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
				{
					LocomotionAnimationPrefixFilter = NewText.ToString();
					SaveSettings();
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				DrawFolderPickerSection(
					LOCTEXT("LocomotionAnimationSourceRootLabel", "Locomotion Animation Source Root"),
					&SCharacterDataAssetTool::LocomotionAnimationSourceRoot,
					LOCTEXT("AutoFillLocomotionAnimationSetButton", "Auto Fill Locomotion Set"),
					&SCharacterDataAssetTool::AutoFillLocomotionAnimationSet,
					false)
			]
		]
	];
}

void SCharacterDataAssetTool::LoadSettings()
{
	if (!GConfig)
	{
		return;
	}

	if (const UEUW_Settings* Settings = UEUW_Settings::Get())
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
			StaticLoadObject(ULastFPSLocomotionAnimationSet::StaticClass(), nullptr, *LocomotionAnimationSetObjectPath));
	}
}

void SCharacterDataAssetTool::SaveSettings() const
{
	if (UEUW_Settings* Settings = GetMutableDefault<UEUW_Settings>())
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
		       : LOCTEXT("NoRowSelected", "Select a row");
}

FReply SCharacterDataAssetTool::OpenFolderPicker(FString SCharacterDataAssetTool::*OutputRootMember)
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
		.Title(LOCTEXT("OutputRootPickerTitle", "Choose Character Asset Folder"))
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
			.Text(LOCTEXT("UseFolderButton", "Use Selected Folder"))
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
			Definition = NewObject<ULastFPSCharacterDefinition>(
				Package,
				ULastFPSCharacterDefinition::StaticClass(),
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
	FString SCharacterDataAssetTool::*OutputRootMember,
	const FText& GenerateButtonText,
	FReply (SCharacterDataAssetTool::*OnGenerateClicked)(),
	bool bRequiresCharacterRow)
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(LabelText)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 16.f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SEditableTextBox)
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
				.Text(LOCTEXT("PickFolderButton", "Choose Folder"))
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
				.ContentPadding(FMargin(12.f, 4.f))
				.Text(GenerateButtonText)
				.IsEnabled_Lambda([this, OutputRootMember, bRequiresCharacterRow]()
				{
					return (!bRequiresCharacterRow || CanGenerate()) && !(this->*OutputRootMember).IsEmpty();
				})
				.OnClicked(this, OnGenerateClicked)
			]
		];
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
			StaticLoadObject(ULastFPSLocomotionAnimationSet::StaticClass(), nullptr, *LocomotionAnimationSetObjectPath));
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
	Definition->DisplayName = FText::FromString(Row.DisplayName);
	Definition->Role = FText::FromString(Row.Role);
	Definition->Description = FText::FromString(Row.Description);
	Definition->Icon = Cast<UTexture2D>(Row.Icon.LoadSynchronous());
	Definition->PawnClass = Row.PawnClass.LoadSynchronous();
	Definition->StatData = Row.StatData.LoadSynchronous();
	Definition->VisualData = Row.VisualData.LoadSynchronous();
	Definition->AbilitySet = Row.AbilitySet.LoadSynchronous();
	Definition->AIProfile = Row.AIProfile.LoadSynchronous();
}

#undef LOCTEXT_NAMESPACE
