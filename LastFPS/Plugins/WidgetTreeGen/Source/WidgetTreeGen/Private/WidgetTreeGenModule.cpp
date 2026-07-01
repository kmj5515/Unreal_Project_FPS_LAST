// Copyright Epic Games, Inc. All Rights Reserved.

#include "WidgetTreeGenModule.h"
#include "WidgetTreeGenerator.h"
#include "WidgetTreeGenRequest.h"
#include "NPCPresetGenLibrary.h"

#include "Engine/DataTable.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UnrealType.h"

#include "Core/EditorUtility.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SWindow.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "DetailsViewArgs.h"
#include "IDetailsView.h"
#include "UObject/StrongObjectPtr.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#define LOCTEXT_NAMESPACE "FWidgetTreeGenModule"

namespace
{
	void ShowResultNotification(const FWidgetTreeGenResult& Result)
	{
		FNotificationInfo Info(Result.bSuccess
			? FText::Format(LOCTEXT("GenSuccess", "Generated: {0}"), FText::FromString(Result.AssetPath))
			: FText::Format(LOCTEXT("GenFailed", "Generation failed: {0}"), FText::FromString(Result.ErrorMessage)));
		Info.ExpireDuration = Result.bSuccess ? 4.0f : 8.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void FWidgetTreeGenModule::StartupModule()
{
	// Append our entries to the EditorUtility plugin's "LastFPS" main-menu submenu.
	FEditorUtilityModule::OnExtendLastFPSMenu().AddRaw(this, &FWidgetTreeGenModule::ExtendLastFPSMenu);
}

void FWidgetTreeGenModule::ShutdownModule()
{
	FEditorUtilityModule::OnExtendLastFPSMenu().RemoveAll(this);
}

void FWidgetTreeGenModule::ExtendLastFPSMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("WidgetTreeGen", LOCTEXT("WidgetTreeGenSection", "Widget Tree Gen"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("OpenGeneratorLabel", "위젯 트리 생성기"),
			LOCTEXT("OpenGeneratorTooltip", "JSON으로 위젯 블루프린트를 생성하는 패널을 엽니다."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.WidgetBlueprint"),
			FUIAction(FExecuteAction::CreateRaw(this, &FWidgetTreeGenModule::OpenGeneratorWindow)));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("GenerateFromFileLabel", "JSON 파일에서 생성..."),
			LOCTEXT("GenerateFromFileTooltip", ".json 파일을 골라 즉시 위젯 블루프린트를 생성합니다."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.WidgetBlueprint"),
			FUIAction(FExecuteAction::CreateRaw(this, &FWidgetTreeGenModule::GenerateFromFileDialog)));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("GenerateFromFolderLabel", "JSON 폴더에서 일괄 생성..."),
			LOCTEXT("GenerateFromFolderTooltip", "폴더를 골라 그 안의 모든 .json을 한 번에 생성합니다."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.WidgetBlueprint"),
			FUIAction(FExecuteAction::CreateRaw(this, &FWidgetTreeGenModule::GenerateFromFolderDialog)));
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("NPCPresetGen", LOCTEXT("NPCPresetGenSection", "NPC Gen"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("GenNPCLabel", "NPC 프리셋 생성 (DT_NPCData)"),
			LOCTEXT("GenNPCTooltip", "Project Settings의 DT_NPCData 각 행마다 BP_<행이름> NPC 블루프린트를 생성합니다."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Blueprint"),
			FUIAction(FExecuteAction::CreateRaw(this, &FWidgetTreeGenModule::GenerateNPCPresets)));
	}
	MenuBuilder.EndSection();
}

void FWidgetTreeGenModule::GenerateNPCPresets()
{
	// Project Settings → Game → LastFPS NPC 의 NPCDataTable 을 리플렉션으로 읽는다
	// (게임 모듈에 링크하지 않기 위해 클래스 경로 + 프로퍼티 리플렉션 사용).
	UDataTable* NPCTable = nullptr;
	if (UClass* SettingsClass = LoadObject<UClass>(nullptr, TEXT("/Script/LastFPS.LastFPSNPCSettings")))
	{
		if (const UObject* Settings = SettingsClass->GetDefaultObject())
		{
			if (const FSoftObjectProperty* Prop = FindFProperty<FSoftObjectProperty>(SettingsClass, TEXT("NPCDataTable")))
			{
				const FSoftObjectPtr& SoftPtr = Prop->GetPropertyValue_InContainer(Settings);
				NPCTable = Cast<UDataTable>(SoftPtr.ToSoftObjectPath().TryLoad());
			}
		}
	}

	if (!NPCTable)
	{
		FNotificationInfo Info(LOCTEXT("NPCNoTable",
			"DT_NPCData를 찾지 못했습니다. Project Settings → Game → LastFPS NPC 에서 NPCDataTable을 지정하세요."));
		Info.ExpireDuration = 8.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	const FNPCPresetGenSummary Summary =
		UNPCPresetGenLibrary::GenerateFromTable(NPCTable, TEXT("/Game/NPC"), TEXT("BP_"), /*bOverwriteExisting=*/false);

	FNotificationInfo Info(FText::FromString(Summary.Message));
	Info.ExpireDuration = 6.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	UE_LOG(LogTemp, Log, TEXT("[NPCPresetGen] %s"), *Summary.Message);
}

void FWidgetTreeGenModule::OpenGeneratorWindow()
{
	static TStrongObjectPtr<UWidgetTreeGenRequest> Request;
	if (!Request.IsValid())
	{
		Request.Reset(NewObject<UWidgetTreeGenRequest>(GetTransientPackage(), NAME_None, RF_Transactional));
	}

	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs Args;
	Args.bAllowSearch = false;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	TSharedRef<IDetailsView> DetailsView = PropertyModule.CreateDetailView(Args);
	DetailsView->SetObject(Request.Get());

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("GeneratorWindowTitle", "Widget Tree Generator"))
		.ClientSize(FVector2D(560.f, 680.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			DetailsView
		];

	FSlateApplication::Get().AddWindow(Window);
}

void FWidgetTreeGenModule::GenerateFromFileDialog()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	const void* ParentHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	TArray<FString> OutFiles;
	const bool bPicked = DesktopPlatform->OpenFileDialog(
		ParentHandle,
		TEXT("Select a widget hierarchy JSON file"),
		FPaths::ProjectDir(),
		TEXT(""),
		TEXT("JSON files (*.json)|*.json"),
		EFileDialogFlags::None,
		OutFiles);

	if (!bPicked || OutFiles.Num() == 0)
	{
		return;
	}

	const FWidgetTreeGenResult Result = FWidgetTreeGenerator::GenerateFromJsonFile(OutFiles[0]);
	ShowResultNotification(Result);
}

void FWidgetTreeGenModule::GenerateFromFolderDialog()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	const void* ParentHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	FString FolderPath;
	const bool bPicked = DesktopPlatform->OpenDirectoryDialog(
		ParentHandle,
		TEXT("Select a folder containing widget JSON files"),
		FPaths::ProjectDir(),
		FolderPath);

	if (!bPicked || FolderPath.IsEmpty())
	{
		return;
	}

	TArray<FString> JsonFiles;
	IFileManager::Get().FindFiles(JsonFiles, *(FolderPath / TEXT("*.json")), /*Files=*/true, /*Directories=*/false);
	if (JsonFiles.Num() == 0)
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("NoJson", "No .json files found in {0}"), FText::FromString(FolderPath)));
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	int32 NumOk = 0;
	TArray<FString> Failures;
	for (const FString& FileName : JsonFiles)
	{
		const FString FullPath = FolderPath / FileName;
		const FWidgetTreeGenResult Result = FWidgetTreeGenerator::GenerateFromJsonFile(FullPath);
		if (Result.bSuccess)
		{
			++NumOk;
		}
		else
		{
			Failures.Add(FString::Printf(TEXT("%s: %s"), *FileName, *Result.ErrorMessage));
		}
	}

	FString Summary = FString::Printf(TEXT("생성 완료: %d/%d"), NumOk, JsonFiles.Num());
	if (Failures.Num() > 0)
	{
		Summary += TEXT("\n실패:\n") + FString::Join(Failures, TEXT("\n"));
	}
	FNotificationInfo Info(FText::FromString(Summary));
	Info.ExpireDuration = Failures.Num() > 0 ? 10.0f : 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	UE_LOG(LogTemp, Log, TEXT("[WidgetTreeGen] Batch: %s"), *Summary);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWidgetTreeGenModule, WidgetTreeGen)
