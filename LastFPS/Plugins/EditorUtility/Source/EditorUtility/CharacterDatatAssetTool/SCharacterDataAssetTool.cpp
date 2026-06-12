#include "SCharacterDataAssetTool.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/LastFPSAbilitySet.h"
#include "Character/LastFPSAIProfile.h"
#include "Character/LastFPSCharacterMasterData.h"
#include "Character/LastFPSCharacterStatData.h"
#include "Character/LastFPSCharacterVisualData.h"
#include "ContentBrowserModule.h"
#include "EUW_Settings.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "Game/LastFPSCharacterDefinition.h"
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
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("OutputRootLabel", "Output Root"))
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
					.Text(this, &SCharacterDataAssetTool::GetOutputRootText)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("PickFolderButton", "Choose Folder"))
					.OnClicked(this, &SCharacterDataAssetTool::OpenOutputRootPicker)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.Text(LOCTEXT("GenerateButton", "Generate"))
				.IsEnabled(this, &SCharacterDataAssetTool::CanGenerate)
				.OnClicked(this, &SCharacterDataAssetTool::GenerateCharacterDefinition)
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
		OutputRoot = Settings->CharacterDefinitionOutputRoot.IsEmpty()
			? OutputRoot
			: Settings->CharacterDefinitionOutputRoot;
		MasterTable = Settings->CharacterMasterTable.LoadSynchronous();
	}

	if (!MasterTable.IsValid() && !MasterTableObjectPath.IsEmpty())
	{
		MasterTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *MasterTableObjectPath));
	}
}

void SCharacterDataAssetTool::SaveSettings() const
{
	if (UEUW_Settings* Settings = GetMutableDefault<UEUW_Settings>())
	{
		Settings->CharacterMasterTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(MasterTableObjectPath));
		Settings->CharacterDefinitionOutputRoot = OutputRoot;
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

FReply SCharacterDataAssetTool::OpenOutputRootPicker()
{
	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = OutputRoot;
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateSP(this, &SCharacterDataAssetTool::HandleOutputRootSelected);

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

void SCharacterDataAssetTool::HandleOutputRootSelected(const FString& SelectedPath)
{
	if (!SelectedPath.IsEmpty())
	{
		OutputRoot = SelectedPath;
		SaveSettings();
	}
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
		const FString PackageName = OutputRoot / AssetName;
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

FText SCharacterDataAssetTool::GetOutputRootText() const
{
	return FText::FromString(OutputRoot);
}

bool SCharacterDataAssetTool::CanGenerate() const
{
	return MasterTable.IsValid() && SelectedRowName.IsValid() && !OutputRoot.IsEmpty();
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
