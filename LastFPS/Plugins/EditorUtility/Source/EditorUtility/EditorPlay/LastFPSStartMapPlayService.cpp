#include "EditorPlay/LastFPSStartMapPlayService.h"

#include "Editor.h"
#include "IAssetViewport.h"
#include "LevelEditor.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/MW_Settings.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

#define LOCTEXT_NAMESPACE "FLastFPSStartMapPlayService"

bool FLastFPSStartMapPlayService::CanPlayConfiguredStartMap()
{
	const FString StartMapPackageName = GetConfiguredStartMapPackageName();
	return GUnrealEd
		&& !GUnrealEd->IsPlaySessionInProgress()
		&& !StartMapPackageName.IsEmpty()
		&& FPackageName::IsValidLongPackageName(StartMapPackageName);
}

void FLastFPSStartMapPlayService::PlayConfiguredStartMap()
{
	if (!GUnrealEd)
	{
		ShowError(LOCTEXT("EditorMissing", "Unreal Editor is not ready."));
		return;
	}

	if (GUnrealEd->IsPlaySessionInProgress())
	{
		ShowError(LOCTEXT("PlayInProgress", "A Play session is already running or queued."));
		return;
	}

	const FString StartMapPackageName = GetConfiguredStartMapPackageName();
	if (!IsPackageUsableForPIE(StartMapPackageName))
	{
		ShowError(LOCTEXT("InvalidStartMap", "Forced Play Start Map is not configured or the map package does not exist."));
		return;
	}

	FRequestPlaySessionParams SessionParams;
	SessionParams.GlobalMapOverride = StartMapPackageName;

	if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		if (TSharedPtr<IAssetViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport())
		{
			SessionParams.DestinationSlateViewport = ActiveLevelViewport;
		}
	}

	GUnrealEd->RequestPlaySession(SessionParams);
	GUnrealEd->StartQueuedPlaySessionRequest();
}

FString FLastFPSStartMapPlayService::GetConfiguredStartMapPackageName()
{
	const UMW_Settings* Settings = UMW_Settings::Get();
	if (!Settings)
	{
		return FString();
	}

	const FSoftObjectPath StartMapPath = Settings->ForcedPlayStartMap.ToSoftObjectPath();
	return StartMapPath.IsValid() ? StartMapPath.GetLongPackageName() : FString();
}

bool FLastFPSStartMapPlayService::IsPackageUsableForPIE(const FString& PackageName)
{
	return !PackageName.IsEmpty()
		&& FPackageName::IsValidLongPackageName(PackageName)
		&& FPackageName::DoesPackageExist(PackageName);
}

void FLastFPSStartMapPlayService::ShowError(const FText& Message)
{
	FMessageDialog::Open(EAppMsgType::Ok, Message);
}

#undef LOCTEXT_NAMESPACE
