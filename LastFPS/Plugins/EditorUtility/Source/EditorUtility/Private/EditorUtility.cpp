// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditorUtility.h"
#include "ToolMenus.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EUW_Settings.h"
#include "Editor.h"
#include "Containers/Ticker.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Application/SlateApplication.h"
#include "Components/SizeBox.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "FEditorUtilityModule"

const FName FEditorUtilityModule::LevelSelectionTabName("LevelSelectionTool");

void FEditorUtilityModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FEditorUtilityModule::RegisterTabSpawner);
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEditorUtilityModule::RegisterMenus));

	// 스타일 세트 초기화 및 등록
	const FName StyleSetName("LastFPSStyle");
	StyleSetInstance = MakeShareable(new FSlateStyleSet(StyleSetName));
	
	// 아이콘 브러시 설정 (텍스처 로드 방식)
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

void FEditorUtilityModule::RegisterTabSpawner()
{
	if (FGlobalTabmanager::Get()->HasTabSpawner(LevelSelectionTabName))
	{
		UnregisterTabSpawner();
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(LevelSelectionTabName,
		FOnSpawnTab::CreateRaw(this, &FEditorUtilityModule::OnSpawnLevelSelectionTab))
		.SetDisplayName(LOCTEXT("LevelSelectionTabTitle", "Level Selection Tool"))
		.SetIcon(FSlateIcon(StyleSetInstance->GetStyleSetName(), "CatIcon"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FEditorUtilityModule::UnregisterTabSpawner()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LevelSelectionTabName);
}

void FEditorUtilityModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
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
			LOCTEXT("OpenLevelSelectionToolTitle", "레벨 선택 툴"),
			LOCTEXT("OpenLevelSelectionToolTooltip", "Opens the custom level selection tool."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.World"),
			FUIAction(FExecuteAction::CreateRaw(this, &FEditorUtilityModule::OpenLevelSelectionTool))
		);
	}
	MenuBuilder.EndSection();
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
			// 1. 일단 월드를 기반으로 위젯을 생성합니다 (컴파일러 제약 충족)
			UEditorUtilityWidget* WidgetInstance = CreateWidget<UEditorUtilityWidget>(World, WidgetBP->GeneratedClass.Get());
			
			if (WidgetInstance)
			{
				// 2. 💡 생성 직후 Outer를 Subsystem으로 변경하여 맵 패키지 참조 체인을 끊습니다.
				UEditorUtilitySubsystem* Subsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
				WidgetInstance->Rename(nullptr, Subsystem);

				NewTab->SetContent(WidgetInstance->TakeWidget());
			}
		}
	}
	return NewTab;
}

void FEditorUtilityModule::OpenLevelSelectionTool()
{
	if (FGlobalTabmanager::Get()->HasTabSpawner(LevelSelectionTabName))
	{
		FGlobalTabmanager::Get()->TryInvokeTab(LevelSelectionTabName);
	}
	else
	{
		RegisterTabSpawner();
		FGlobalTabmanager::Get()->TryInvokeTab(LevelSelectionTabName);
	}
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FEditorUtilityModule, EditorUtility)
