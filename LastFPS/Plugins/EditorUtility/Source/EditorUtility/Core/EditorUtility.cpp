#include "Core/EditorUtility.h"
#include "BattleLevelTool/SLastFPSBattleLevelTool.h"
#include "CharacterDatatAssetTool/SCharacterDataAssetTool.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "EditorPlay/LastFPSStartMapPlayToolbar.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISettingsContainer.h"
#include "ISettingsModule.h"
#include "Hub/LastFPSNPCSettings.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "RuntimeStats/SLastFPSRuntimeStatsEditor.h"
#include "Settings/LastFPSBattleLevelSettings.h"
#include "Settings/MW_Settings.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FEditorUtilityModule"

#pragma region 툴 이름
const FName FEditorUtilityModule::LevelSelectionTabName("LevelSelectionTool");
const FName FEditorUtilityModule::CharacterDataAssetTabName("CharacterDataAssetTool");
const FName FEditorUtilityModule::RuntimeStatsEditorTabName(TEXT("LastFPS.RuntimeStatsEditor"));
const FName FEditorUtilityModule::BattleLevelToolTabName(TEXT("LastFPS.BattleLevelTool"));
const FName FEditorUtilityModule::ToolName(TEXT("MWTool"));
#pragma endregion

#pragma region 툴 우선 Index
const float FEditorUtilityModule::MWToolPriority = -10.f;
#pragma endregion

FEditorUtilityModule::FOnExtendLastFPSMenu& FEditorUtilityModule::OnExtendLastFPSMenu()
{
	static FOnExtendLastFPSMenu Delegate;
	return Delegate;
}

void FEditorUtilityModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FEditorUtilityModule::RegisterTabSpawner);
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEditorUtilityModule::RegisterMenus));
	RegisterProjectSettings();

	const FName StyleSetName("LastFPSStyle");
	StyleSetInstance = MakeShareable(new FSlateStyleSet(StyleSetName));

	const FString IconPath = TEXT("/EditorUtility/Assets/Icons/CatIcon.CatIcon");
	if (UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, *IconPath))
	{
		StyleSetInstance->Set("CatIcon", new FSlateImageBrush(IconTexture, FVector2D(16.f, 16.f)));
	}

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSetInstance);
}

void FEditorUtilityModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	UnregisterProjectSettings();
	UnregisterTabSpawner();

	if (StyleSetInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSetInstance);
		StyleSetInstance.Reset();
	}
}

void FEditorUtilityModule::RegisterProjectSettings()
{
	ISettingsModule* SettingsModule = FModuleManager::LoadModulePtr<ISettingsModule>(TEXT("Settings"));
	if (!SettingsModule)
	{
		return;
	}

	if (ISettingsContainerPtr ProjectSettingsContainer = SettingsModule->GetContainer(TEXT("Project")))
	{
		ProjectSettingsContainer->DescribeCategory(
			ToolName,
			LOCTEXT("MWToolSettingsCategoryName", "MWTool"),
			LOCTEXT("MWToolSettingsCategoryDescription", "MWTool project settings")
		);
		ProjectSettingsContainer->SetCategorySortPriority(ToolName, MWToolPriority);
	}

	SettingsModule->RegisterSettings(
		TEXT("Project"),
		ToolName,
		TEXT("EditorTools"),
		LOCTEXT("EditorToolsSettingsName", "에디터 관련 구성요소"),
		LOCTEXT("EditorToolsSettingsDescription", "Configure MWTool editor tools."),
		GetMutableDefault<UMW_Settings>()
	);

	SettingsModule->RegisterSettings(
		TEXT("Project"),
		ToolName,
		TEXT("NPC"),
		LOCTEXT("NPCSettingsName", "NPC"),
		LOCTEXT("NPCSettingsDescription", "Configure LastFPS NPC settings."),
		GetMutableDefault<ULastFPSNPCSettings>()
	);

	SettingsModule->RegisterSettings(
		TEXT("Project"),
		ToolName,
		TEXT("BattleLevels"),
		LOCTEXT("BattleLevelsSettingsName", "Battle Levels"),
		LOCTEXT("BattleLevelsSettingsDescription", "Configure LastFPS battle level editor tools."),
		GetMutableDefault<ULastFPSBattleLevelSettings>()
	);
}

void FEditorUtilityModule::UnregisterProjectSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings")))
	{
		SettingsModule->UnregisterSettings(TEXT("Project"), ToolName, TEXT("EditorTools"));
		SettingsModule->UnregisterSettings(TEXT("Project"), ToolName, TEXT("NPC"));
		SettingsModule->UnregisterSettings(TEXT("Project"), ToolName, TEXT("BattleLevels"));
	}
}

void FEditorUtilityModule::OpenRuntimeStatsEditor()
{
	if (!FGlobalTabmanager::Get()->HasTabSpawner(RuntimeStatsEditorTabName)
		&& FModuleManager::Get().IsModuleLoaded(TEXT("EditorUtility")))
	{
		FEditorUtilityModule& Module = FModuleManager::LoadModuleChecked<FEditorUtilityModule>(TEXT("EditorUtility"));
		Module.RegisterTabSpawner();
	}

	FGlobalTabmanager::Get()->TryInvokeTab(RuntimeStatsEditorTabName);
}

void FEditorUtilityModule::RegisterTabSpawner()
{
	UnregisterTabSpawner();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(LevelSelectionTabName,
		FOnSpawnTab::CreateRaw(this, &FEditorUtilityModule::OnSpawnLevelSelectionTab))
		.SetDisplayName(LOCTEXT("LevelSelectionTabTitle", "Level Selection Tool"))
		.SetIcon(FSlateIcon(StyleSetInstance->GetStyleSetName(), "CatIcon"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(CharacterDataAssetTabName,
		FOnSpawnTab::CreateRaw(this, &FEditorUtilityModule::OnSpawnCharacterDataAssetTab))
		.SetDisplayName(LOCTEXT("CharacterDataAssetTabTitle", "Character Data Asset Tool"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.PrimaryDataAsset"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(RuntimeStatsEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FEditorUtilityModule::OnSpawnRuntimeStatsEditorTab))
		.SetDisplayName(LOCTEXT("RuntimeStatsEditorTabTitle", "LastFPS Runtime Stats"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Blueprint"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(BattleLevelToolTabName,
		FOnSpawnTab::CreateRaw(this, &FEditorUtilityModule::OnSpawnBattleLevelToolTab))
		.SetDisplayName(LOCTEXT("BattleLevelToolTabTitle", "Battle Level Tool"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.World"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FEditorUtilityModule::UnregisterTabSpawner()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LevelSelectionTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CharacterDataAssetTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RuntimeStatsEditorTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BattleLevelToolTabName);
}

void FEditorUtilityModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	FLastFPSStartMapPlayToolbar::Register();

	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
	if (MainMenu)
	{
		FToolMenuSection& Section = MainMenu->AddSection("LastFPSCustomSection", LOCTEXT("LastFPSCustomSectionLabel", "LastFPS"));
		Section.AddSubMenu(
			"LastFPSLevelTools",
			LOCTEXT("LastFPSMenuLabel", "LastFPS"),
			LOCTEXT("LastFPSMenuTooltip", "LastFPS Editor Utilities"),
			FNewMenuDelegate::CreateRaw(this, &FEditorUtilityModule::FillLastFPSMenu)
		);
	}
}

void FEditorUtilityModule::FillLastFPSMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("LevelTools", LOCTEXT("LevelToolsSection", "Level Tools"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("OpenLevelSelectionToolTitle", "Level Selection"),
			LOCTEXT("OpenLevelSelectionToolTooltip", "Opens the custom level selection tool."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.World"),
			FUIAction(FExecuteAction::CreateRaw(this, &FEditorUtilityModule::OpenLevelSelectionTool))
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("OpenCharacterDataAssetToolTitle", "Character Data Asset"),
			LOCTEXT("OpenCharacterDataAssetTooltip", "Opens the custom character data asset tool."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.PrimaryDataAsset"),
			FUIAction(FExecuteAction::CreateRaw(this, &FEditorUtilityModule::OpenCharacterDataAssetTool))
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("OpenBattleLevelToolTitle", "Battle Levels"),
			LOCTEXT("OpenBattleLevelToolTooltip", "Opens the battle level editor tool."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.World"),
			FUIAction(FExecuteAction::CreateRaw(this, &FEditorUtilityModule::OpenBattleLevelTool))
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("RuntimeTools", LOCTEXT("RuntimeToolsSection", "Runtime Tools"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("OpenRuntimeStatsToolTitle", "Runtime Stats"),
			LOCTEXT("OpenRuntimeStatsToolTooltip", "Opens the runtime character stats editor."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Blueprint"),
			FUIAction(FExecuteAction::CreateRaw(this, &FEditorUtilityModule::OpenRuntimeStatsTool))
		);
	}
	MenuBuilder.EndSection();

	OnExtendLastFPSMenu().Broadcast(MenuBuilder);
}

TSharedRef<SDockTab> FEditorUtilityModule::OnSpawnLevelSelectionTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> NewTab = SNew(SDockTab).TabRole(ETabRole::NomadTab);
	const UMW_Settings* Settings = UMW_Settings::Get();

	if (Settings && !Settings->LevelSelectionTool.IsNull())
	{
		UEditorUtilityWidgetBlueprint* WidgetBP = Settings->LevelSelectionTool.LoadSynchronous();
		if (WidgetBP && WidgetBP->GeneratedClass)
		{
			UWorld* World = GEditor->GetEditorWorldContext().World();
			UEditorUtilityWidget* WidgetInstance = CreateWidget<UEditorUtilityWidget>(World, WidgetBP->GeneratedClass.Get());

			if (WidgetInstance)
			{
				UEditorUtilitySubsystem* Subsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
				WidgetInstance->Rename(nullptr, Subsystem);
				NewTab->SetContent(WidgetInstance->TakeWidget());
			}
		}
	}

	return NewTab;
}

TSharedRef<SDockTab> FEditorUtilityModule::OnSpawnCharacterDataAssetTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SCharacterDataAssetTool)
		];
}

TSharedRef<SDockTab> FEditorUtilityModule::OnSpawnRuntimeStatsEditorTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SLastFPSRuntimeStatsEditor)
		];
}

TSharedRef<SDockTab> FEditorUtilityModule::OnSpawnBattleLevelToolTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SLastFPSBattleLevelTool)
		];
}

void FEditorUtilityModule::OpenLevelSelectionTool()
{
	if (!FGlobalTabmanager::Get()->HasTabSpawner(LevelSelectionTabName))
	{
		RegisterTabSpawner();
	}

	FGlobalTabmanager::Get()->TryInvokeTab(LevelSelectionTabName);
}

void FEditorUtilityModule::OpenCharacterDataAssetTool()
{
	if (!FGlobalTabmanager::Get()->HasTabSpawner(CharacterDataAssetTabName))
	{
		RegisterTabSpawner();
	}

	FGlobalTabmanager::Get()->TryInvokeTab(CharacterDataAssetTabName);
}

void FEditorUtilityModule::OpenRuntimeStatsTool()
{
	OpenRuntimeStatsEditor();
}

void FEditorUtilityModule::OpenBattleLevelTool()
{
	if (!FGlobalTabmanager::Get()->HasTabSpawner(BattleLevelToolTabName))
	{
		RegisterTabSpawner();
	}

	FGlobalTabmanager::Get()->TryInvokeTab(BattleLevelToolTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEditorUtilityModule, EditorUtility)
