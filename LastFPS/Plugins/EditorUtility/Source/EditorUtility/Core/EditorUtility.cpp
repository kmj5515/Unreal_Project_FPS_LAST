#include "Core/EditorUtility.h"

#include "CharacterDatatAssetTool/SCharacterDataAssetTool.h"
#include "Components/SizeBox.h"
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
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "RuntimeStats/SLastFPSRuntimeStatsEditor.h"
#include "Settings/EUW_Settings.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FEditorUtilityModule"

const FName FEditorUtilityModule::LevelSelectionTabName("LevelSelectionTool");
const FName FEditorUtilityModule::CharacterDataAssetTabName("CharacterDataAssetTool");
const FName FEditorUtilityModule::RuntimeStatsEditorTabName(TEXT("LastFPS.RuntimeStatsEditor"));

FEditorUtilityModule::FOnExtendLastFPSMenu& FEditorUtilityModule::OnExtendLastFPSMenu()
{
	static FOnExtendLastFPSMenu Delegate;
	return Delegate;
}

void FEditorUtilityModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FEditorUtilityModule::RegisterTabSpawner);
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEditorUtilityModule::RegisterMenus));

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
	UnregisterTabSpawner();

	if (StyleSetInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSetInstance);
		StyleSetInstance.Reset();
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
}

void FEditorUtilityModule::UnregisterTabSpawner()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LevelSelectionTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CharacterDataAssetTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RuntimeStatsEditorTabName);
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
	const UEUW_Settings* Settings = UEUW_Settings::Get();

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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEditorUtilityModule, EditorUtility)
